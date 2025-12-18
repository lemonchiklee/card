#ifndef CARDDIALOG_H
#define CARDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "card.h"

class CardDialog : public QDialog {
    Q_OBJECT
public:
    explicit CardDialog(QWidget* parent = nullptr);
    
    void setCard(const Card& card);
    Card getCard() const;

private slots:
    void onSelectImage();
    void onRemoveImage();
    void onSave();

private:
    void setupUi();
    void applyStyle();
    void populateCombos();
    void updateImagePreview();
    
    qint64 m_cardId = 0;
    QByteArray m_imageData;
    
    QLineEdit* m_nameEdit;
    QComboBox* m_participantCombo;
    QComboBox* m_sourceCombo;
    QComboBox* m_albumCombo;
    QLabel* m_imagePreview;
    QPushButton* m_selectImgBtn;
    QPushButton* m_removeImgBtn;
    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;
};

#endif
