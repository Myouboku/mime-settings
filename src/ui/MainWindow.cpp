#include "ui/MainWindow.h"

#include "models/MimeTypeFilterProxy.h"
#include "models/MimeTypeModel.h"
#include "ui/DetailsPane.h"
#include "utils/XdgPaths.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPair>
#include <QPen>
#include <QPixmap>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeView>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {
QIcon makeEmojiIcon(const QString &emoji) {
  const int size = 18;
  QPixmap pixmap(size, size);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::TextAntialiasing, true);

  QFont font = painter.font();
  font.setPixelSize(size - 2);
  painter.setFont(font);
  painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, emoji);

  return QIcon(pixmap);
}

QIcon makeAccentIcon(const QColor &color) {
  QPixmap pixmap(14, 14);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  QColor border = color.darker(120);
  painter.setPen(QPen(border, 1));
  painter.setBrush(color);
  painter.drawEllipse(QRectF(1, 1, 12, 12));

  return QIcon(pixmap);
}

QColor blendColors(const QColor &top, const QColor &bottom, double alpha) {
  alpha = std::clamp(alpha, 0.0, 1.0);
  auto mix = [alpha](int topValue, int bottomValue) -> int {
    return static_cast<int>(std::round(bottomValue + (topValue - bottomValue) * alpha));
  };
  return QColor(mix(top.red(), bottom.red()), mix(top.green(), bottom.green()),
                mix(top.blue(), bottom.blue()));
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_service(&m_registry, &m_store) {
  m_registry.load();
  m_store.reload();
  loadPalette();
  loadAppearanceSettings();
  buildUi();
  loadData();
}

void MainWindow::buildUi() {
  setWindowTitle("MIME Settings");
  resize(1100, 720);

  auto *content = new QWidget(this);
  auto *contentLayout = new QVBoxLayout(content);
  contentLayout->setContentsMargins(16, 16, 16, 16);
  contentLayout->setSpacing(12);

  auto *header = new QWidget(content);
  header->setObjectName("AppHeader");
  auto *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(14, 10, 14, 10);
  headerLayout->setSpacing(10);

  auto *headerTitle = new QLabel("MIME Settings", header);
  headerTitle->setObjectName("HeaderTitle");
  QFont headerFont = headerTitle->font();
  headerFont.setPointSize(headerFont.pointSize() + 2);
  headerFont.setBold(true);
  headerTitle->setFont(headerFont);

  auto *themeLabel = new QLabel("Theme", header);
  themeLabel->setObjectName("HeaderLabel");
  m_themePicker = new QComboBox(header);
  m_themePicker->setObjectName("ThemePicker");
  m_themePicker->setMinimumWidth(160);

  auto *accentLabel = new QLabel("Accent", header);
  accentLabel->setObjectName("HeaderLabel");
  m_accentPicker = new QComboBox(header);
  m_accentPicker->setObjectName("AccentPicker");
  m_accentPicker->setMinimumWidth(160);

  headerLayout->addWidget(headerTitle);
  headerLayout->addStretch(1);
  headerLayout->addWidget(themeLabel);
  headerLayout->addWidget(m_themePicker);
  headerLayout->addSpacing(6);
  headerLayout->addWidget(accentLabel);
  headerLayout->addWidget(m_accentPicker);

  contentLayout->addWidget(header);

  auto *splitter = new QSplitter(content);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(1);

  auto *leftPane = new QWidget(splitter);
  leftPane->setObjectName("LeftPane");
  auto *leftLayout = new QVBoxLayout(leftPane);
  leftLayout->setContentsMargins(0, 0, 8, 0);
  leftLayout->setSpacing(10);

  m_search = new QLineEdit(leftPane);
  m_search->setPlaceholderText("Search MIME types or applications");
  m_search->setObjectName("SearchField");
  m_search->setClearButtonEnabled(true);
  m_search->setFocus();

  m_table = new QTreeView(leftPane);
  m_table->setObjectName("MimeTable");
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setAlternatingRowColors(true);
  m_table->setSortingEnabled(true);
  m_table->setUniformRowHeights(true);
  m_table->setIndentation(14);
  m_table->header()->setStretchLastSection(false);
  m_table->header()->setSectionResizeMode(QHeaderView::Interactive);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setRootIsDecorated(true);
  m_table->setItemsExpandable(true);
  m_table->setExpandsOnDoubleClick(true);

  leftLayout->addWidget(m_search);
  leftLayout->addWidget(m_table, 1);

  m_details = new DetailsPane(&m_registry, splitter);
  m_details->setObjectName("DetailsPane");

  splitter->addWidget(leftPane);
  splitter->addWidget(m_details);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);

  contentLayout->addWidget(splitter, 1);
  setCentralWidget(content);
  setStatusBar(new QStatusBar(this));

  m_model = new MimeTypeModel(&m_registry, this);
  m_proxy = new MimeTypeFilterProxy(this);
  m_proxy->setSourceModel(m_model);
  m_proxy->setDynamicSortFilter(true);
  m_proxy->sort(MimeTypeModel::MimeColumn, Qt::AscendingOrder);
  m_table->setModel(m_proxy);

  m_table->header()->setSectionResizeMode(MimeTypeModel::MimeColumn, QHeaderView::Interactive);
  m_table->header()->setSectionResizeMode(MimeTypeModel::DefaultAppColumn,
                                          QHeaderView::Interactive);
  m_table->header()->setSectionResizeMode(MimeTypeModel::DescriptionColumn, QHeaderView::Stretch);
  m_table->setColumnWidth(MimeTypeModel::MimeColumn, 240);
  m_table->setColumnWidth(MimeTypeModel::DefaultAppColumn, 220);
  m_table->header()->setSortIndicator(MimeTypeModel::MimeColumn, Qt::AscendingOrder);
  m_table->sortByColumn(MimeTypeModel::MimeColumn, Qt::AscendingOrder);

  connect(m_search, &QLineEdit::textChanged, this, [this](const QString &text) {
    m_proxy->setFilterText(text);
    if (!text.trimmed().isEmpty()) {
      m_table->expandAll();
    }
  });
  connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &MainWindow::onSelectionChanged);
  connect(m_details, &DetailsPane::requestSetDefault, this, &MainWindow::onRequestSetDefault);

  populateThemePicker();
  populateAccentPicker();
  applyTheme();

  connect(m_themePicker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
    if (m_updatingAppearance) {
      return;
    }

    const QString themeId = m_themePicker->currentData().toString();
    if (themeId.isEmpty() || themeId == m_currentThemeId) {
      return;
    }

    m_currentThemeId = themeId;
    populateAccentPicker();
    applyTheme();
    saveAppearanceSettings();
  });

  connect(m_accentPicker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
    if (m_updatingAppearance) {
      return;
    }

    const QString accentId = m_accentPicker->currentData().toString();
    if (accentId.isEmpty() || accentId == m_currentAccentId) {
      return;
    }

    m_currentAccentId = accentId;
    applyTheme();
    saveAppearanceSettings();
  });
}

void MainWindow::loadPalette() {
  m_themes.clear();
  m_themeOrder.clear();

  QFile file(":/assets/palette.json");
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }

  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
  if (doc.isNull() || !doc.isObject()) {
    return;
  }

  QJsonObject root = doc.object();
  QVector<QPair<int, QString>> order;

  for (auto it = root.begin(); it != root.end(); ++it) {
    if (!it.value().isObject()) {
      continue;
    }

    QJsonObject themeObj = it.value().toObject();
    if (!themeObj.contains("colors")) {
      continue;
    }

    ThemeData theme;
    theme.id = it.key();
    theme.name = themeObj.value("name").toString(theme.id);
    theme.emoji = themeObj.value("emoji").toString();
    theme.dark = themeObj.value("dark").toBool();
    theme.order = themeObj.value("order").toInt(0);

    QJsonObject colorsObj = themeObj.value("colors").toObject();
    for (auto colorIt = colorsObj.begin(); colorIt != colorsObj.end(); ++colorIt) {
      if (!colorIt.value().isObject()) {
        continue;
      }

      QJsonObject colorObj = colorIt.value().toObject();
      ThemeColor color;
      color.id = colorIt.key();
      color.name = colorObj.value("name").toString(color.id);
      color.hex = colorObj.value("hex").toString();
      color.accent = colorObj.value("accent").toBool();
      color.order = colorObj.value("order").toInt(0);

      theme.colors.insert(color.id, color);
      if (color.accent) {
        theme.accents.append(color);
      }
    }

    std::sort(theme.accents.begin(), theme.accents.end(),
              [](const ThemeColor &a, const ThemeColor &b) { return a.order < b.order; });

    m_themes.insert(theme.id, theme);
    order.append({theme.order, theme.id});
  }

  std::sort(
      order.begin(), order.end(),
      [](const QPair<int, QString> &a, const QPair<int, QString> &b) { return a.first < b.first; });

  for (const auto &pair : order) {
    m_themeOrder.append(pair.second);
  }

  if (m_themes.isEmpty()) {
    ThemeData fallback;
    fallback.id = "mocha";
    fallback.name = "Mocha";
    fallback.dark = true;
    fallback.order = 0;

    auto addColor = [&fallback](const QString &id, const QString &name, const QString &hex,
                                bool accent, int order) {
      ThemeColor color;
      color.id = id;
      color.name = name;
      color.hex = hex;
      color.accent = accent;
      color.order = order;
      fallback.colors.insert(id, color);
      if (accent) {
        fallback.accents.append(color);
      }
    };

    addColor("base", "Base", "#1e1e2e", false, 0);
    addColor("mantle", "Mantle", "#181825", false, 1);
    addColor("crust", "Crust", "#11111b", false, 2);
    addColor("surface0", "Surface 0", "#313244", false, 3);
    addColor("surface1", "Surface 1", "#45475a", false, 4);
    addColor("surface2", "Surface 2", "#585b70", false, 5);
    addColor("overlay0", "Overlay 0", "#6c7086", false, 6);
    addColor("overlay1", "Overlay 1", "#7f849c", false, 7);
    addColor("overlay2", "Overlay 2", "#9399b2", false, 8);
    addColor("text", "Text", "#cdd6f4", false, 9);
    addColor("subtext0", "Subtext 0", "#a6adc8", false, 10);
    addColor("subtext1", "Subtext 1", "#bac2de", false, 11);
    addColor("mauve", "Mauve", "#cba6f7", true, 0);

    std::sort(fallback.accents.begin(), fallback.accents.end(),
              [](const ThemeColor &a, const ThemeColor &b) { return a.order < b.order; });

    m_themes.insert(fallback.id, fallback);
    m_themeOrder.append(fallback.id);
  }
}

void MainWindow::loadAppearanceSettings() {
  const QString fallbackTheme = m_themes.contains("mocha")
                                    ? QString("mocha")
                                    : (m_themeOrder.isEmpty() ? QString() : m_themeOrder.first());
  const QString fallbackAccent = "mauve";

  QSettings settings(settingsFilePath(), QSettings::IniFormat);
  m_currentThemeId = settings.value("appearance/theme", fallbackTheme).toString();
  if (!m_themes.contains(m_currentThemeId)) {
    m_currentThemeId = fallbackTheme;
  }

  m_currentAccentId = settings.value("appearance/accent", fallbackAccent).toString();
}

void MainWindow::saveAppearanceSettings() const {
  QSettings settings(settingsFilePath(), QSettings::IniFormat);
  settings.setValue("appearance/theme", m_currentThemeId);
  settings.setValue("appearance/accent", m_currentAccentId);
}

QString MainWindow::settingsFilePath() const {
  const QString dirPath = XdgPaths::configHome() + "/mime-settings";
  QDir dir(dirPath);
  if (!dir.exists()) {
    dir.mkpath(".");
  }
  return dirPath + "/settings.ini";
}

void MainWindow::populateThemePicker() {
  if (!m_themePicker) {
    return;
  }

  QSignalBlocker blocker(m_themePicker);
  m_themePicker->clear();

  for (const QString &themeId : m_themeOrder) {
    const ThemeData theme = m_themes.value(themeId);
    const QString label = theme.name.isEmpty() ? themeId : theme.name;
    if (!theme.emoji.isEmpty()) {
      m_themePicker->addItem(makeEmojiIcon(theme.emoji), label, themeId);
    } else {
      m_themePicker->addItem(label, themeId);
    }
  }

  int index = m_themePicker->findData(m_currentThemeId);
  if (index < 0 && m_themePicker->count() > 0) {
    index = 0;
    m_currentThemeId = m_themePicker->itemData(index).toString();
  }

  if (index >= 0) {
    m_themePicker->setCurrentIndex(index);
  }
}

void MainWindow::populateAccentPicker() {
  if (!m_accentPicker) {
    return;
  }

  const ThemeData *theme = currentTheme();
  if (!theme) {
    return;
  }

  QSignalBlocker blocker(m_accentPicker);
  m_accentPicker->clear();

  for (const ThemeColor &accent : theme->accents) {
    QColor color(accent.hex);
    QIcon icon = makeAccentIcon(color);
    const QString label = accent.name.isEmpty() ? accent.id : accent.name;
    m_accentPicker->addItem(icon, label, accent.id);
  }

  int index = m_accentPicker->findData(m_currentAccentId);
  if (index < 0 && !theme->accents.isEmpty()) {
    m_currentAccentId = theme->accents.first().id;
    index = m_accentPicker->findData(m_currentAccentId);
  }

  if (index >= 0) {
    m_accentPicker->setCurrentIndex(index);
  }
}

const MainWindow::ThemeData *MainWindow::currentTheme() const {
  auto it = m_themes.constFind(m_currentThemeId);
  if (it != m_themes.constEnd()) {
    return &it.value();
  }

  if (!m_themeOrder.isEmpty()) {
    auto fallback = m_themes.constFind(m_themeOrder.first());
    if (fallback != m_themes.constEnd()) {
      return &fallback.value();
    }
  }

  return nullptr;
}

const MainWindow::ThemeColor *MainWindow::currentAccent(const ThemeData &theme) const {
  auto it = theme.colors.constFind(m_currentAccentId);
  if (it != theme.colors.constEnd()) {
    return &it.value();
  }

  if (!theme.accents.isEmpty()) {
    return &theme.accents.first();
  }

  return nullptr;
}

QString MainWindow::colorFor(const ThemeData &theme, const QString &id,
                             const QString &fallback) const {
  auto it = theme.colors.constFind(id);
  if (it != theme.colors.constEnd() && !it.value().hex.isEmpty()) {
    return it.value().hex;
  }

  return fallback;
}

void MainWindow::applyTheme() {
  const ThemeData *theme = currentTheme();
  if (!theme) {
    return;
  }

  const ThemeColor *accent = currentAccent(*theme);
  if (!accent) {
    return;
  }

  const QString base = colorFor(*theme, "base", "#1e1e2e");
  const QString mantle = colorFor(*theme, "mantle", "#181825");
  const QString crust = colorFor(*theme, "crust", "#11111b");
  const QString surface0 = colorFor(*theme, "surface0", "#313244");
  const QString surface1 = colorFor(*theme, "surface1", "#45475a");
  const QString surface2 = colorFor(*theme, "surface2", "#585b70");
  const QString overlay1 = colorFor(*theme, "overlay1", "#7f849c");
  const QString overlay2 = colorFor(*theme, "overlay2", "#9399b2");
  const QString text = colorFor(*theme, "text", "#cdd6f4");
  const QString subtext0 = colorFor(*theme, "subtext0", "#a6adc8");
  const QString subtext1 = colorFor(*theme, "subtext1", "#bac2de");

  QColor accentColor(accent->hex);
  const QString accentHex = accentColor.name();
  const QString accentHover = accentColor.lighter(112).name();
  const QString accentPressed = accentColor.darker(110).name();
  const QString hoverBg = blendColors(accentColor, QColor(surface0), 0.22).name();
  const QString selectionBg = theme->dark ? accentColor.darker(135).name() : accentHex;
  const QString selectionText = theme->dark ? text : base;
  const QString groupHeaderBg =
      theme->dark ? QColor(surface0).darker(115).name() : QColor(surface0).lighter(110).name();

  QString style;
  style += QString("QMainWindow { background: qlineargradient(x1:0, y1:0, x2:1, "
                   "y2:1, stop:0 %1, stop:1 %2); }\n")
               .arg(mantle, base);
  style += QString("QWidget { color: %1; }\n").arg(text);
  style += QString("#AppHeader { background: %1; border: 1px solid %2; "
                   "border-radius: 12px; }\n")
               .arg(surface0, surface1);
  style += QString("#HeaderTitle { color: %1; }\n").arg(text);
  style += QString("#HeaderLabel { color: %1; }\n").arg(subtext0);
  style += QString("QStatusBar { background: %1; color: %2; border-top: 1px solid "
                   "%3; }\n")
               .arg(mantle, subtext1, surface1);
  style += QString("QLineEdit, QComboBox { background: %1; border: 1px solid %2; "
                   "border-radius: 8px; padding: 6px 10px; }\n")
               .arg(surface0, surface1);
  style += QString("QLineEdit:focus, QComboBox:focus { border: 1px solid %1; }\n").arg(accentHex);
  style += QString("QLineEdit::placeholder { color: %1; }\n").arg(subtext0);
  style += QString("QComboBox::drop-down { border-left: 1px solid %1; "
                   "width: 22px; }\n")
               .arg(surface1);
  style += QString("QComboBox QAbstractItemView { background: %1; border: 1px "
                   "solid %2; selection-background-color: %3; "
                   "selection-color: %4; }\n")
               .arg(surface0, surface1, accentHex, selectionText);
  style += QString("QTreeView, QListWidget, DetailsPane { background: %1; border: "
                   "1px solid %2; border-radius: 12px; gridline-color: %3; }\n")
               .arg(surface0, surface1, surface2);
  style += QString("QHeaderView::section { background: %1; padding: 6px; border: "
                   "none; border-bottom: 1px solid %2; color: %3; }\n")
               .arg(surface1, surface2, subtext1);
  style += QString("QTreeView::item { padding: 6px 8px; }\n");
  style += QString("QTreeView::item:alternate { background: %1; }\n").arg(surface1);
  style += QString("QTreeView::item:has-children { background: %1; }\n").arg(groupHeaderBg);
  style += QString("QTreeView::item:selected { background: %1; color: %2; }\n")
               .arg(selectionBg, selectionText);
  style += QString("QListWidget::item:selected { background: %1; color: %2; "
                   "border-radius: 6px; }\n")
               .arg(selectionBg, selectionText);
  style += QString("QTreeView::item:hover, QListWidget::item:hover { background: "
                   "%1; }\n")
               .arg(hoverBg);
  style += QString("QListWidget::item:hover { border-radius: 6px; }\n");
  style += QString("QListWidget::item { padding: 6px; margin: 2px 4px; }\n");
  style += QString("QPushButton { background: %1; color: %2; border: none; "
                   "border-radius: 8px; padding: 8px 16px; font-weight: 600; }\n")
               .arg(accentHex, selectionText);
  style += QString("QPushButton:hover { background: %1; }\n").arg(accentHover);
  style += QString("QPushButton:pressed { background: %1; }\n").arg(accentPressed);
  style += QString("QPushButton:disabled { background: %1; color: %2; }\n").arg(surface2, overlay2);
  style += QString("QSplitter::handle { background: %1; }\n").arg(surface2);
  style += QString("QScrollBar:vertical { background: %1; width: 12px; margin: 0; "
                   "} QScrollBar::handle:vertical { background: %2; min-height: "
                   "24px; border-radius: 6px; } "
                   "QScrollBar::handle:vertical:hover { background: %3; } "
                   "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
                   "height: 0; }\n")
               .arg(surface0, surface2, overlay1);
  style += QString("QScrollBar:horizontal { background: %1; height: 12px; margin: "
                   "0; } QScrollBar::handle:horizontal { background: %2; "
                   "min-width: 24px; border-radius: 6px; } "
                   "QScrollBar::handle:horizontal:hover { background: %3; } "
                   "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal "
                   "{ width: 0; }\n")
               .arg(surface0, surface2, overlay1);
  style += QString("QToolTip { background: %1; color: %2; border: 1px solid %3; "
                   "border-radius: 6px; padding: 6px 8px; }\n")
               .arg(surface0, text, surface1);
  style += QString("QLabel#DetailsTitle { color: %1; }\n").arg(text);
  style += QString("QLabel#DetailsDescription { color: %1; }\n").arg(subtext1);
  style += QString("QLabel#DetailsHint { color: %1; }\n").arg(subtext0);
  style += QString("QLabel#SectionLabel { color: %1; font-weight: 600; }\n").arg(subtext0);

  setStyleSheet(style);
}

void MainWindow::loadData(const QString &preserveMime) {
  const QVector<MimeEntry> entries = m_service.buildEntries();
  m_model->setEntries(entries);
  m_table->sortByColumn(MimeTypeModel::MimeColumn, Qt::AscendingOrder);

  if (!preserveMime.isEmpty()) {
    selectMime(preserveMime);
  } else if (m_proxy->rowCount() > 0) {
    selectFirstEntry();
  } else {
    m_details->setEntry(MimeEntry{});
  }
}

void MainWindow::selectMime(const QString &mime) {
  const QModelIndex sourceIndex = m_model->indexForMime(mime);
  const QModelIndex proxyIndex = m_proxy->mapFromSource(sourceIndex);

  if (proxyIndex.isValid()) {
    if (proxyIndex.parent().isValid()) {
      m_table->expand(proxyIndex.parent());
    }
    m_table->setCurrentIndex(proxyIndex);
    m_table->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect |
                                                      QItemSelectionModel::Rows);
    m_table->scrollTo(proxyIndex);
  }
}

void MainWindow::selectFirstEntry() {
  for (int i = 0; i < m_proxy->rowCount(); ++i) {
    const QModelIndex categoryIndex = m_proxy->index(i, 0);
    if (!categoryIndex.isValid()) {
      continue;
    }

    const int childCount = m_proxy->rowCount(categoryIndex);
    if (childCount == 0) {
      continue;
    }

    m_table->expand(categoryIndex);
    const QModelIndex firstChild = m_proxy->index(0, 0, categoryIndex);
    if (firstChild.isValid()) {
      m_table->setCurrentIndex(firstChild);
      m_table->selectionModel()->select(firstChild, QItemSelectionModel::ClearAndSelect |
                                                        QItemSelectionModel::Rows);
      m_table->scrollTo(firstChild);
      return;
    }
  }

  m_details->setEntry(MimeEntry{});
}

void MainWindow::onSelectionChanged() {
  const QModelIndexList selection = m_table->selectionModel()->selectedRows();

  if (selection.isEmpty()) {
    m_details->setEntry(MimeEntry{});
    return;
  }

  const QModelIndex proxyIndex = selection.first();
  const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
  const MimeEntry entry = m_model->entryForIndex(sourceIndex);
  m_details->setEntry(entry);
}

void MainWindow::onRequestSetDefault(const QString &mime, const QString &desktopId) {
  m_service.setDefault(mime, desktopId);
  statusBar()->showMessage(QString("Default updated for %1").arg(mime), 3000);
  loadData(mime);
}
