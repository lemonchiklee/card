#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include "card.h"
#include "database.h"

class CardWidget : public QFrame {
    Q_OBJECT
public:
    explicit CardWidget(const CardWithStatus& cws, bool isAdmin, QWidget* parent = nullptr);
    
    void setData(const CardWithStatus& cws);
    CardWithStatus data() const { return m_data; }
    qint64 cardId() const { return m_data.card.id; }
    
    void setSelected(bool sel);
    bool isSelected() const { return m_selected; }
    void setImageExpanded(bool exp);

signals:
    void favoriteClicked(qint64 id);
    void obtainedClicked(qint64 id);
    void editClicked(qint64 id);
    void deleteClicked(qint64 id);
    void selectionChanged(qint64 id, bool sel);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void enterEvent(QEvent* e) override;
    void leaveEvent(QEvent* e) override;

private slots:
    void onFavorite();
    void onObtained();
    void onToggleImage();
    void onEdit();
    void onDelete();

private:
    void setupUi();
    void updateDisplay();
    void updateStyle();
    
    CardWithStatus m_data;
    bool m_isAdmin;
    bool m_selected = false;
    bool m_expanded = false;
    bool m_hovered = false;
    
    QLabel* m_nameLabel;
    QLabel* m_participantLabel;
    QLabel* m_sourceLabel;
    QLabel* m_albumLabel;
    QLabel* m_imagePreview;
    
    QPushButton* m_favBtn;
    QPushButton* m_obtBtn;
    QPushButton* m_imgToggle;
    QPushButton* m_editBtn;
    QPushButton* m_delBtn;
    
    QWidget* m_imageBox;
};

#endif
