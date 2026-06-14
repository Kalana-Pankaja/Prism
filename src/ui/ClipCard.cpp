#include "ui/ClipCard.h"
#include "ui_ClipCard.h"
#include "ui/ClipEditDialog.h"
#include <QFileInfo>
#include <QFontMetrics>

ClipCard::ClipCard(int index, QWidget *parent)
    : QFrame(parent), m_index(index), ui(new Ui::ClipCard) {
    ui->setupUi(this);

    connect(ui->thumbnailBtn, &QPushButton::clicked, this, [this]() {
        if (!m_clipPath.isEmpty()) emit triggered(m_index);
    });
    connect(ui->muteBtn,   &QPushButton::clicked, this, &ClipCard::onMuteClicked);
    connect(ui->repeatBtn, &QPushButton::clicked, this, &ClipCard::onRepeatClicked);
    connect(ui->editBtn,   &QPushButton::clicked, this, &ClipCard::onEditClicked);
    connect(ui->aBtn,      &QPushButton::clicked, this, &ClipCard::onAButtonClicked);
    connect(ui->bBtn,      &QPushButton::clicked, this, &ClipCard::onBButtonClicked);

    clearClip();
}

ClipCard::~ClipCard() {
    delete ui;
}

int ClipCard::volume() const {
    return ui->volumeSlider->value();
}

void ClipCard::loadClip(const QString &path, const QPixmap &thumbnail) {
    m_clipPath = path;
    m_settings = ClipSettings::loadFor(path);  // load sidecar (or defaults)

    if (!thumbnail.isNull()) {
        ui->thumbnailBtn->setIcon(QIcon(thumbnail));
        ui->thumbnailBtn->setText("");
    } else {
        ui->thumbnailBtn->setIcon(QIcon());
        ui->thumbnailBtn->setText("No Preview");
    }

    QFileInfo info(path);
    QString name = info.baseName();
    QFontMetrics fm(ui->titleLabel->font());
    ui->titleLabel->setText(fm.elidedText(name, Qt::ElideRight, 108));
    ui->titleLabel->setToolTip(name);

    ui->repeatBtn->setEnabled(true);
    ui->editBtn->setEnabled(true);
    ui->muteBtn->setEnabled(true);
    ui->volumeSlider->setEnabled(true);
    ui->aBtn->setEnabled(true);
    ui->bBtn->setEnabled(true);
    setActive(false);
}

void ClipCard::clearClip() {
    m_clipPath.clear();
    m_settings = {};
    ui->thumbnailBtn->setIcon(QIcon());
    ui->thumbnailBtn->setText("Empty");
    ui->titleLabel->setText("—");
    ui->titleLabel->setToolTip({});
    ui->repeatBtn->setEnabled(false);
    ui->editBtn->setEnabled(false);
    ui->muteBtn->setEnabled(false);
    ui->volumeSlider->setEnabled(false);
    ui->aBtn->setEnabled(false);
    ui->bBtn->setEnabled(false);
    setActive(false);
    setASelected(false);
    setBSelected(false);
}

void ClipCard::setActive(bool active) {
    if (active) {
        setStyleSheet("ClipCard { background-color: #1a3d45; border: 2px solid #2a8fa0; border-radius: 8px; }");
    } else {
        setStyleSheet("ClipCard { background-color: #242528; border: 1px solid #1c1d1f; border-radius: 8px; }");
    }
}

void ClipCard::setASelected(bool selected) {
    m_aSelected = selected;
    if (selected) {
        ui->aBtn->setStyleSheet("QPushButton { background-color: #2a5c66; color: #FFFFFF; font-weight: bold; font-size: 9px; min-height: 0; height: 20px; border-radius: 4px; }");
    } else {
        ui->aBtn->setStyleSheet("font-size: 9px; min-height: 0; height: 20px;");
    }
}

void ClipCard::setBSelected(bool selected) {
    m_bSelected = selected;
    if (selected) {
        ui->bBtn->setStyleSheet("QPushButton { background-color: #2a5c66; color: #FFFFFF; font-weight: bold; font-size: 9px; min-height: 0; height: 20px; border-radius: 4px; }");
    } else {
        ui->bBtn->setStyleSheet("font-size: 9px; min-height: 0; height: 20px;");
    }
}

void ClipCard::onMuteClicked() {
    m_muted = !m_muted;
    ui->muteBtn->setText(m_muted ? "🔇" : "🔊");
}

void ClipCard::onRepeatClicked() {
    m_repeat = !m_repeat;
    ui->repeatBtn->setText(m_repeat ? "↺  On" : "↺  Off");
}

void ClipCard::onEditClicked() {
    if (m_clipPath.isEmpty()) return;
    ClipEditDialog dlg(m_clipPath, m_settings, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_settings = dlg.resultSettings();
        m_settings.saveFor(m_clipPath);
    }
}

void ClipCard::onAButtonClicked() {
    if (m_clipPath.isEmpty()) return;
    emit aButtonClicked(m_index);
}

void ClipCard::onBButtonClicked() {
    if (m_clipPath.isEmpty()) return;
    emit bButtonClicked(m_index);
}
