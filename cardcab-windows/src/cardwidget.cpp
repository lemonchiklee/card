#include "cardwidget.h"
#include "constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPixmap>

CardWidget::CardWidget(const CardWithStatus& cws, bool isAdmin, QWidget* parent)
    : QFrame(parent), m_data(cws), m_isAdmin(isAdmin) {
    setupUi();
    updateDisplay();
    updateStyle();
}

void CardWidget::setupUi() {
    setFrameStyle(QFrame::StyledPanel);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(100);
    
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(8);
    main->setContentsMargins(12, 12, 12, 12);
    
    // Header: name + favorite button
    QHBoxLayout* header = new QHBoxLayout();
    
    m_nameLabel = new QLabel();
    m_nameLabel->setObjectName("name");
    m_nameLabel->setWordWrap(true);
    header->addWidget(m_nameLabel, 1);
    
    m_favBtn = new QPushButton();
    m_favBtn->setObjectName("favBtn");
    m_favBtn->setFixedSize(40, 40);
    m_favBtn->setCursor(Qt::PointingHandCursor);
    m_favBtn->setToolTip("★ Избранное (желаемое)");
    header->addWidget(m_favBtn);
    
    main->addLayout(header);
    
    // Details
    QVBoxLayout* details = new QVBoxLayout();
    details->setSpacing(4);
    
    m_participantLabel = new QLabel();
    m_participantLabel->setObjectName("detail");
    details->addWidget(m_participantLabel);
    
    m_sourceLabel = new QLabel();
    m_sourceLabel->setObjectName("detail");
    details->addWidget(m_sourceLabel);
    
    m_albumLabel = new QLabel();
    m_albumLabel->setObjectName("detail");
    m_albumLabel->setWordWrap(true);
    details->addWidget(m_albumLabel);
    
    main->addLayout(details);
    
    // Status button
    QHBoxLayout* statusRow = new QHBoxLayout();
    
    m_obtBtn = new QPushButton();
    m_obtBtn->setObjectName("statusBtn");
    m_obtBtn->setCursor(Qt::PointingHandCursor);
    statusRow->addWidget(m_obtBtn);
    
    statusRow->addStretch();
    main->addLayout(statusRow);
    
    // Image container (hidden by default)
    m_imageBox = new QWidget();
    m_imageBox->setVisible(false);
    QVBoxLayout* imgLayout = new QVBoxLayout(m_imageBox);
    imgLayout->setContentsMargins(0, 8, 0, 0);
    
    m_imagePreview = new QLabel();
    m_imagePreview->setObjectName("imgPreview");
    m_imagePreview->setAlignment(Qt::AlignCenter);
    m_imagePreview->setMinimumSize(200, 150);
    m_imagePreview->setMaximumSize(400, 300);
    imgLayout->addWidget(m_imagePreview, 0, Qt::AlignCenter);
    
    main->addWidget(m_imageBox);
    
    // Image toggle button
    m_imgToggle = new QPushButton("Показать изображение");
    m_imgToggle->setObjectName("toggleBtn");
    m_imgToggle->setCursor(Qt::PointingHandCursor);
    main->addWidget(m_imgToggle);
    
    // Admin buttons
    QHBoxLayout* actions = new QHBoxLayout();
    actions->addStretch();
    
    m_editBtn = new QPushButton("Редактировать");
    m_editBtn->setObjectName("actionBtn");
    m_editBtn->setCursor(Qt::PointingHandCursor);
    m_editBtn->setVisible(m_isAdmin);
    actions->addWidget(m_editBtn);
    
    m_delBtn = new QPushButton("Удалить");
    m_delBtn->setObjectName("delBtn");
    m_delBtn->setCursor(Qt::PointingHandCursor);
    m_delBtn->setVisible(m_isAdmin);
    actions->addWidget(m_delBtn);
    
    main->addLayout(actions);
    
    connect(m_favBtn, &QPushButton::clicked, this, &CardWidget::onFavorite);
    connect(m_obtBtn, &QPushButton::clicked, this, &CardWidget::onObtained);
    connect(m_imgToggle, &QPushButton::clicked, this, &CardWidget::onToggleImage);
    connect(m_editBtn, &QPushButton::clicked, this, &CardWidget::onEdit);
    connect(m_delBtn, &QPushButton::clicked, this, &CardWidget::onDelete);
}

void CardWidget::setData(const CardWithStatus& cws) {
    m_data = cws;
    updateDisplay();
}

void CardWidget::updateDisplay() {
    m_nameLabel->setText(m_data.card.cardName);
    
    QString part = m_data.card.participantName.isEmpty() ? "-" : m_data.card.participantName;
    m_participantLabel->setText("Участник: " + part);
    
    QString src = m_data.card.source.isEmpty() ? "-" : m_data.card.source;
    m_sourceLabel->setText("Источник: " + src);
    
    QString alb = m_data.card.albumName.isEmpty() ? "-" : m_data.card.albumName;
    m_albumLabel->setText("Альбом: " + alb);
    
    // Favorite button - при нажатии: тусклый жёлтый фон с яркой жёлтой звездой
    if (m_data.status.isFavorite) {
        m_favBtn->setText(QChar(0x2605));  // ★ Black Star (filled)
        m_favBtn->setStyleSheet(
            "QPushButton { background: #4a4522; "  // Тусклый жёлтый фон
            "color: #ffd700; border-radius: 20px; "  // Яркая жёлтая звезда
            "font-size: 22px; font-weight: bold; border: 2px solid #ffd700; }"  // Жёлтая обводка
            "QPushButton:hover { background: #5a5532; border: 2px solid #ffed4a; }"  // При наведении чуть ярче
        );
    } else {
        m_favBtn->setText(QChar(0x2605));  // ★ Black Star (not favorited - gray bg)
        m_favBtn->setStyleSheet(
            "QPushButton { background: #3d3d5c; color: #5d5d7c; border-radius: 20px; "  // Серый фон, тёмная звезда
            "font-size: 22px; border: 2px solid #4d4d6c; }"
            "QPushButton:hover { background: #3d3d5c; color: #5d5d7c; "
            "border: 2px solid #ffd700; }"  // При наведении только жёлтая обводка
        );
    }
    
    // Obtained button
    if (m_data.status.isObtained) {
        m_obtBtn->setText("Получено [V]");
        m_obtBtn->setStyleSheet(
            "QPushButton { background: #2ecc71; color: white; border-radius: 4px; padding: 8px 16px; font-weight: bold; }"
            "QPushButton:hover { background: #27ae60; }"
        );
    } else {
        m_obtBtn->setText("Не получено");
        m_obtBtn->setStyleSheet(
            "QPushButton { background: #e74c3c; color: white; border-radius: 4px; padding: 8px 16px; }"
            "QPushButton:hover { background: #c0392b; }"
        );
    }
    
    // Image
    if (!m_data.card.imageData.isEmpty()) {
        QPixmap pix;
        pix.loadFromData(m_data.card.imageData);
        m_imagePreview->setPixmap(pix.scaled(m_imagePreview->maximumSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imgToggle->setVisible(true);
    } else {
        m_imagePreview->setText("Нет изображения");
        m_imgToggle->setVisible(false);
        m_imageBox->setVisible(false);
    }
}

void CardWidget::updateStyle() {
    QString bg = m_selected ? "#3d4d6c" : (m_hovered ? "#2d3d5c" : "#2d2d44");
    QString border = m_selected ? "#ff4757" : "#3d3d5c";
    
    setStyleSheet(QString(
        "CardWidget { background: %1; border: 2px solid %2; border-radius: 10px; }"
        "QLabel#name { color: #e0e0e0; font-size: 16px; font-weight: bold; }"
        "QLabel#detail { color: #7f8c8d; font-size: 12px; }"
        "QLabel#imgPreview { background: #1a1b2e; border-radius: 6px; padding: 10px; color: #7f8c8d; }"
        "QPushButton#toggleBtn { background: transparent; color: #7f8c8d; border: 1px solid #3d3d5c; border-radius: 4px; padding: 6px; }"
        "QPushButton#toggleBtn:hover { background: #3d3d5c; color: #e0e0e0; }"
        "QPushButton#actionBtn { background: #3d3d5c; color: #e0e0e0; border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton#actionBtn:hover { background: #4d4d6c; }"
        "QPushButton#delBtn { background: #e74c3c; color: white; border: none; border-radius: 4px; padding: 6px 12px; }"
        "QPushButton#delBtn:hover { background: #c0392b; }"
    ).arg(bg, border));
}

void CardWidget::setSelected(bool sel) {
    if (m_selected != sel) {
        m_selected = sel;
        updateStyle();
        emit selectionChanged(m_data.card.id, sel);
    }
}

void CardWidget::setImageExpanded(bool exp) {
    m_expanded = exp;
    m_imageBox->setVisible(exp && !m_data.card.imageData.isEmpty());
    m_imgToggle->setText(exp ? "Скрыть изображение" : "Показать изображение");
}

void CardWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) setSelected(!m_selected);
    QFrame::mousePressEvent(e);
}

void CardWidget::enterEvent(QEvent* e) {
    m_hovered = true;
    updateStyle();
    QFrame::enterEvent(e);
}

void CardWidget::leaveEvent(QEvent* e) {
    m_hovered = false;
    updateStyle();
    QFrame::leaveEvent(e);
}

void CardWidget::onFavorite() { emit favoriteClicked(m_data.card.id); }
void CardWidget::onObtained() { emit obtainedClicked(m_data.card.id); }
void CardWidget::onToggleImage() { setImageExpanded(!m_expanded); }
void CardWidget::onEdit() { emit editClicked(m_data.card.id); }
void CardWidget::onDelete() { emit deleteClicked(m_data.card.id); }
