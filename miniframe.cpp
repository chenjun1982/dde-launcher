#include "miniframe.h"
#include "dbusdock.h"
#include "widgets/miniframenavigation.h"
#include "widgets/searchlineedit.h"
#include "widgets/minicategorywidget.h"
#include "view/appgridview.h"
#include "view/applistview.h"
#include "model/appslistmodel.h"
#include "delegate/appitemdelegate.h"
#include "delegate/applistdelegate.h"

#include <QRect>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QScrollArea>

#define DOCK_TOP        0
#define DOCK_RIGHT      1
#define DOCK_BOTTOM     2
#define DOCK_LEFT       3

MiniFrame::MiniFrame(QWidget *parent)
    : DBlurEffectWidget(parent),

      m_dockInter(new DBusDock(this)),
      m_appsManager(AppsManager::instance()),

      m_navigation(new MiniFrameNavigation),
      m_categoryWidget(new MiniCategoryWidget),

      m_appsView(nullptr),
      m_appsModel(new AppsListModel(AppsListModel::All)),
      m_searchModel(new AppsListModel(AppsListModel::Search))
{
    m_navigation->setFixedWidth(140);
    m_categoryWidget->setFixedWidth(140);
    m_categoryWidget->setVisible(false);

    m_viewToggle = new DImageButton;
    m_viewToggle->setNormalPic(":/icons/skin/icons/category_normal_22px.svg");
    m_modeToggle = new DImageButton;
    m_modeToggle->setNormalPic(":/icons/skin/icons/fullscreen_normal.png");
    m_searchEdit = new SearchLineEdit;

    QHBoxLayout *viewHeaderLayout = new QHBoxLayout;
    viewHeaderLayout->addWidget(m_viewToggle);
    viewHeaderLayout->addStretch();
    viewHeaderLayout->addWidget(m_searchEdit);
    viewHeaderLayout->addStretch();
    viewHeaderLayout->addWidget(m_modeToggle);
    viewHeaderLayout->setSpacing(0);
    viewHeaderLayout->setMargin(0);

    m_appsBox = new DVBoxWidget;

    m_appsArea = new QScrollArea;
    m_appsArea->setWidget(m_appsBox);
    m_appsArea->setWidgetResizable(true);
    m_appsArea->setFocusPolicy(Qt::NoFocus);
    m_appsArea->setFrameStyle(QFrame::NoFrame);
    m_appsArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_appsArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_appsArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_appsArea->setStyleSheet("background-color: transparent;");

    QHBoxLayout *appsAreaLayout = new QHBoxLayout;
    appsAreaLayout->addWidget(m_categoryWidget);
    appsAreaLayout->addWidget(m_appsArea);
    appsAreaLayout->setSpacing(0);
    appsAreaLayout->setMargin(0);

    QVBoxLayout *viewLayout = new QVBoxLayout;
    viewLayout->addLayout(viewHeaderLayout);
    viewLayout->addLayout(appsAreaLayout);
    viewLayout->setSpacing(0);
    viewLayout->setMargin(0);

    m_viewWrapper = new QWidget;
    m_viewWrapper->setLayout(viewLayout);
    m_viewWrapper->setObjectName("ViewWrapper");
    m_viewWrapper->setStyleSheet("QWidget #ViewWrapper {"
                                 "background-color: rgba(255, 255, 255, .1);"
                                 "border-radius: 8px;"
                                 "}");

    QHBoxLayout *centralLayout = new QHBoxLayout;
    centralLayout->addWidget(m_navigation);
    centralLayout->addWidget(m_viewWrapper);
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(0, 10, 10, 10);

    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMaskColor(DBlurEffectWidget::DarkColor);
    setFixedSize(640, 480);
    setBlurRectXRadius(5);
    setBlurRectYRadius(5);
    setLayout(centralLayout);

    connect(m_searchEdit, &SearchLineEdit::textChanged, this, &MiniFrame::searchText, Qt::QueuedConnection);
    connect(m_modeToggle, &DImageButton::clicked, this, &MiniFrame::toggleFullScreen, Qt::QueuedConnection);
    connect(m_viewToggle, &DImageButton::clicked, this, &MiniFrame::onToggleViewClicked, Qt::QueuedConnection);
    connect(m_categoryWidget, &MiniCategoryWidget::requestCategory, m_appsModel, &AppsListModel::setCategory, Qt::QueuedConnection);

    QTimer::singleShot(1, this, &MiniFrame::toggleAppsView);
}

MiniFrame::~MiniFrame()
{

}

void MiniFrame::showLauncher()
{
    if (visible())
        return;

    connect(m_dockInter, &DBusDock::FrontendRectChanged, this, &MiniFrame::adjustPosition, Qt::QueuedConnection);
    QTimer::singleShot(1, this, &MiniFrame::adjustPosition);

    show();
}

void MiniFrame::hideLauncher()
{
    if (!visible())
        return;

    disconnect(m_dockInter, &DBusDock::FrontendRectChanged, this, &MiniFrame::adjustPosition);

    hide();
}

bool MiniFrame::visible()
{
    return isVisible();
}

void MiniFrame::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);

    if (e->button() == Qt::LeftButton)
        hide();
}

void MiniFrame::keyPressEvent(QKeyEvent *e)
{
    QWidget::keyPressEvent(e);

    switch (e->key())
    {
    case Qt::Key_Escape:    hideLauncher();     break;
    default:;
    }
}

void MiniFrame::showEvent(QShowEvent *e)
{
    DBlurEffectWidget::showEvent(e);

    QTimer::singleShot(1, this, [this] () {
        raise();
        activateWindow();
    });
}

bool MiniFrame::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate && isVisible())
        hideLauncher();

    return QWidget::event(event);
}

void MiniFrame::adjustPosition()
{
    const int dockPos = m_dockInter->position();
    const QRect dockRect = m_dockInter->frontendRect();

    const int spacing = 10;
    const QSize s = size();
    QPoint p;

    switch (dockPos)
    {
    case DOCK_TOP:
        p = QPoint(dockRect.left(), dockRect.bottom() + spacing);
        break;
    case DOCK_BOTTOM:
        p = QPoint(dockRect.left(), dockRect.top() - s.height() - spacing);
        break;
    case DOCK_LEFT:
        p = QPoint(dockRect.right() + spacing, dockRect.top());
        break;
    case DOCK_RIGHT:
        p = QPoint(dockRect.left() - s.width() - spacing, dockRect.top());
        break;
    default: Q_UNREACHABLE_IMPL();
    }

    move(p);
}

void MiniFrame::toggleAppsView()
{
    delete m_appsView;
    m_appsView = nullptr;

    CalculateUtil *calc = CalculateUtil::instance();

    if (calc->displayMode() == ALL_APPS)
    {
        AppGridView *appsView = new AppGridView;
        appsView->setModel(m_appsModel);
        appsView->setItemDelegate(new AppItemDelegate);
        appsView->setContainerBox(m_appsArea);
        appsView->setSpacing(0);
        m_appsView = appsView;

        m_categoryWidget->setVisible(false);
        m_appsModel->setCategory(AppsListModel::All);
    } else {
        AppListView *appsView = new AppListView;
        appsView->setModel(m_appsModel);
        appsView->setItemDelegate(new AppListDelegate);
        m_appsView = appsView;

        m_categoryWidget->setVisible(true);
        m_appsModel->setCategory(AppsListModel::All);
    }

    connect(m_appsView, &QListView::clicked, m_appsManager, &AppsManager::launchApp, Qt::QueuedConnection);
    connect(m_appsView, &QListView::clicked, this, &MiniFrame::hideLauncher, Qt::QueuedConnection);

    m_appsBox->layout()->addWidget(m_appsView);

    CalculateUtil::instance()->calculateAppLayout(QSize(), 0);
}

void MiniFrame::toggleFullScreen()
{
    const QStringList args {
        "--print-reply",
        "--dest=com.deepin.dde.daemon.Launcher",
        "/com/deepin/dde/daemon/Launcher",
        "org.freedesktop.DBus.Properties.Set",
        "string:com.deepin.dde.daemon.Launcher",
        "string:Fullscreen",
        "variant:boolean:true"
    };

    QProcess::startDetached("dbus-send", args);
}

void MiniFrame::onToggleViewClicked()
{
    CalculateUtil *calc = CalculateUtil::instance();
    const int mode = calc->displayMode();

    calc->setDisplayMode(mode == ALL_APPS ? GROUP_BY_CATEGORY : ALL_APPS);

    QTimer::singleShot(1, this, &MiniFrame::toggleAppsView);
}

void MiniFrame::searchText(const QString &text)
{
    if (text.isEmpty())
    {
        m_appsView->setModel(m_appsModel);
    } else {
        m_appsManager->searchApp(text.trimmed());
        m_appsView->setModel(m_searchModel);
    }
}
