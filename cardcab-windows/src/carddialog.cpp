#include "carddialog.h"
#include "constants.h"
#include "database.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QFile>

CardDialog::CardDialog(QWidget* parent) : QDialog(parent) {
    setupUi();
    applyStyle();
    populateCombos();
    setWindowTitle("Добавить карту");
    setMinimumSize(500, 550);
}

void CardDialog::setupUi() {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(15);
    main->setContentsMargins(20, 20, 20, 20);
    
    QLabel* title = new QLabel("Данные карты");
    title->setObjectName("dialogTitle");
    main->addWidget(title);
    
    QFormLayout* form = new QFormLayout();
    form->setSpacing(12);
    
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Название карты (обязательно)");
    m_nameEdit->setMinimumHeight(35);
    form->addRow("Название*:", m_nameEdit);
    
    m_participantCombo = new QComboBox();
    m_participantCombo->setEditable(false);  // Only allow selection from list
    m_participantCombo->setMinimumHeight(35);
    form->addRow("Участник:", m_participantCombo);
    
    m_sourceCombo = new QComboBox();
    m_sourceCombo->setEditable(false);  // Only allow selection from list
    m_sourceCombo->setMinimumHeight(35);
    form->addRow("Источник:", m_sourceCombo);
    
    m_albumCombo = new QComboBox();
    m_albumCombo->setEditable(false);  // Only allow selection from list
    m_albumCombo->setMinimumHeight(35);
    form->addRow("Альбом:", m_albumCombo);
    
    main->addLayout(form);
    
    // Image group
    QGroupBox* imgGroup = new QGroupBox("Изображение");
    QVBoxLayout* imgLayout = new QVBoxLayout(imgGroup);
    
    m_imagePreview = new QLabel();
    m_imagePreview->setObjectName("imgPreview");
    m_imagePreview->setFixedSize(200, 150);
    m_imagePreview->setAlignment(Qt::AlignCenter);
    m_imagePreview->setText("Нет изображения");
    imgLayout->addWidget(m_imagePreview, 0, Qt::AlignCenter);
    
    QHBoxLayout* imgBtns = new QHBoxLayout();
    m_selectImgBtn = new QPushButton("Выбрать PNG");
    imgBtns->addWidget(m_selectImgBtn);
    m_removeImgBtn = new QPushButton("Удалить");
    imgBtns->addWidget(m_removeImgBtn);
    imgLayout->addLayout(imgBtns);
    
    main->addWidget(imgGroup);
    main->addStretch();
    
    // Buttons
    QHBoxLayout* btns = new QHBoxLayout();
    btns->setSpacing(15);
    
    m_cancelBtn = new QPushButton("Отмена");
    m_cancelBtn->setMinimumHeight(40);
    btns->addWidget(m_cancelBtn);
    
    m_saveBtn = new QPushButton("Сохранить");
    m_saveBtn->setObjectName("primary");
    m_saveBtn->setMinimumHeight(40);
    btns->addWidget(m_saveBtn);
    
    main->addLayout(btns);
    
    connect(m_selectImgBtn, &QPushButton::clicked, this, &CardDialog::onSelectImage);
    connect(m_removeImgBtn, &QPushButton::clicked, this, &CardDialog::onRemoveImage);
    connect(m_saveBtn, &QPushButton::clicked, this, &CardDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CardDialog::applyStyle() {
    setStyleSheet(R"(
        QDialog { background: #1a1b2e; }
        QLabel { color: #e0e0e0; font-size: 13px; }
        QLabel#dialogTitle { font-size: 18px; font-weight: bold; color: #ff4757; }
        QLabel#imgPreview { background: #2d2d44; border: 2px dashed #3d3d5c; border-radius: 8px; color: #7f8c8d; }
        QLineEdit, QComboBox { padding: 10px; border: 2px solid #3d3d5c; border-radius: 6px; background: #2d2d44; color: #e0e0e0; }
        QLineEdit:focus, QComboBox:focus { border-color: #ff4757; }
        QComboBox QAbstractItemView { background: #2d2d44; color: #e0e0e0; selection-background-color: #ff4757; }
        QGroupBox { color: #e0e0e0; font-weight: bold; border: 2px solid #3d3d5c; border-radius: 8px; margin-top: 12px; padding-top: 12px; }
        QGroupBox::title { subcontrol-origin: margin; left: 15px; padding: 0 8px; }
        QPushButton { background: #3d3d5c; color: #e0e0e0; border: none; padding: 10px 20px; border-radius: 6px; }
        QPushButton:hover { background: #4d4d6c; }
        QPushButton#primary { background: #ff4757; color: white; font-weight: bold; }
        QPushButton#primary:hover { background: #ff6b81; }
    )");
}

void CardDialog::populateCombos() {
    m_participantCombo->clear();
    m_participantCombo->addItem("");
    for (const QString& p : App::PARTICIPANTS) m_participantCombo->addItem(p);
    
    m_sourceCombo->clear();
    m_sourceCombo->addItem("");
    for (const QString& s : Database::get().getAllSources()) m_sourceCombo->addItem(s);
    
    m_albumCombo->clear();
    m_albumCombo->addItem("");
    for (const QString& a : Database::get().getAllAlbums()) m_albumCombo->addItem(a);
}

void CardDialog::setCard(const Card& card) {
    m_cardId = card.id;
    m_imageData = card.imageData;
    
    m_nameEdit->setText(card.cardName);
    
    // Find participant in combo, default to first item (empty) if not found
    int idx = m_participantCombo->findText(card.participantName);
    m_participantCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    
    // Find source in combo
    idx = m_sourceCombo->findText(card.source);
    m_sourceCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    
    // Find album in combo
    idx = m_albumCombo->findText(card.albumName);
    m_albumCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    
    updateImagePreview();
    setWindowTitle(m_cardId == 0 ? "Добавить карту" : "Редактировать карту");
}

Card CardDialog::getCard() const {
    Card c;
    c.id = m_cardId;
    c.cardName = m_nameEdit->text().trimmed();
    c.participantName = m_participantCombo->currentText().trimmed();
    c.source = m_sourceCombo->currentText().trimmed();
    c.albumName = m_albumCombo->currentText().trimmed();
    c.imageData = m_imageData;
    return c;
}

void CardDialog::updateImagePreview() {
    if (m_imageData.isEmpty()) {
        m_imagePreview->setPixmap(QPixmap());
        m_imagePreview->setText("Нет изображения");
    } else {
        QPixmap pix;
        pix.loadFromData(m_imageData);
        m_imagePreview->setPixmap(pix.scaled(m_imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imagePreview->setText("");
    }
}

void CardDialog::onSelectImage() {
    // Use cross-platform home directory
    QString path = QFileDialog::getOpenFileName(this, "Выберите PNG", App::getDefaultPath(), "PNG (*.png);;All (*)");
    if (path.isEmpty()) return;
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
        return;
    }
    
    m_imageData = file.readAll();
    file.close();
    
    QPixmap test;
    if (!test.loadFromData(m_imageData)) {
        QMessageBox::warning(this, "Ошибка", "Некорректное изображение");
        m_imageData.clear();
        return;
    }
    updateImagePreview();
}

void CardDialog::onRemoveImage() {
    m_imageData.clear();
    updateImagePreview();
}

void CardDialog::onSave() {
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название карты");
        m_nameEdit->setFocus();
        return;
    }
    
    // Since combos are non-editable, values are always valid from the lists
    // Just accept
    accept();
}
