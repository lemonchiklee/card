#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSet>
#include <QMap>

#include "user.h"
#include "card.h"
#include "database.h"
#include "cardwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const User& user, QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onAddCard();
    void onEditCard(qint64 id);
    void onDeleteCard(qint64 id);
    void onToggleFavorite(qint64 id);
    void onToggleObtained(qint64 id);
    void onSelectionChanged(qint64 id, bool sel);
    void onApplyFilters();
    void onClearFilters();
    void onToggleFilterPanel();
    void onExportSelected();
    void onExportAll();
    void onImport();
    void onUserImport();
    void onViewBinaryFile();
    void onChangeAdminCreds();
    void onAddCustomSource();
    void onAddCustomAlbum();
    void onDeleteSources();
    void onDeleteAlbums();
    void onLogout();
    void onRefresh();
    void onAbout();

private:
    void setupUi();
    void setupMenu();
    void setupToolbar();
    void setupFilterPanel();
    void setupCardList();
    void setupStatusBar();
    void applyStyle();
    void loadCards();
    void displayCards(const QVector<CardWithStatus>& cards);
    void clearCards();
    void updateStatus();
    CardFilter buildFilter() const;
    QString getHostPath(const QString& title, bool save = false, const QString& filter = QString());
    
    User m_user;
    QVector<CardWithStatus> m_cards;
    QMap<qint64, CardWidget*> m_widgets;
    QSet<qint64> m_selected;
    QString m_lastExport;
    bool m_logoutRequested = false;
    
    QWidget* m_central;
    QVBoxLayout* m_mainLayout;
    
    QWidget* m_filterPanel;
    QLineEdit* m_searchEdit;
    QCheckBox* m_filterObtainedCheck;
    QCheckBox* m_filterNotObtainedCheck;
    QComboBox* m_filterParticipantCombo;
    QComboBox* m_filterSourceCombo;
    QComboBox* m_filterAlbumCombo;
    QCheckBox* m_filterFavoriteCheck;
    QPushButton* m_toggleFilterBtn;
    
    QScrollArea* m_scroll;
    QWidget* m_cardContainer;
    QVBoxLayout* m_cardLayout;
    
    QLabel* m_userLabel;
    QLabel* m_countLabel;
    QLabel* m_selectedLabel;
    QLabel* m_statsLabel;
};

#endif
