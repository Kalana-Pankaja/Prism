#include "ui/QmlEditDialog.h"
#include "ui_QmlEditDialog.h"
#include "core/DynamicInterfaceSource.h"
#include <QFile>
#include <QFont>
#include <QImage>
#include <QCoreApplication>

namespace {

struct Preset { const char *name; const char *resource; };

static const Preset kPresets[] = {
    { "Clock",               ":/qml/clock.qml"               },
    { "Countdown Timer",     ":/qml/countdown_timer.qml"     },
    { "Cricket Score Bar",   ":/qml/cricket_score_bar.qml"   },
    { "Cricket Score Table", ":/qml/cricket_score_table.qml" },
};
static const int kPresetCount = static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));

static QString loadResource(const char *path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}

} // namespace

QmlEditDialog::QmlEditDialog(const QString &initialCode, QWidget *parent)
    : QDialog(parent), ui(new Ui::QmlEditDialog)
{
    ui->setupUi(this);

    QFont mono("Monospace", 9);
    mono.setStyleHint(QFont::Monospace);
    mono.setFixedPitch(true);
    ui->codeEdit->setFont(mono);
    ui->codeEdit->setTabStopDistance(28);

    for (int i = 0; i < kPresetCount; ++i)
        ui->presetList->addItem(kPresets[i].name);
    ui->presetList->addItem("Custom QML");

    connect(ui->presetList,      &QListWidget::currentRowChanged,
            this,                &QmlEditDialog::onPresetSelected);
    connect(ui->refreshPreviewBtn, &QPushButton::clicked,
            this,                &QmlEditDialog::onRefreshPreview);
    connect(ui->buttonBox,       &QDialogButtonBox::accepted,
            this,                &QDialog::accept);
    connect(ui->buttonBox,       &QDialogButtonBox::rejected,
            this,                &QDialog::reject);

    if (!initialCode.isEmpty()) {
        ui->codeEdit->setPlainText(initialCode);
        QString trimmed = initialCode.trimmed();
        int matchRow = kPresetCount;
        for (int i = 0; i < kPresetCount; ++i) {
            if (loadResource(kPresets[i].resource) == trimmed) {
                matchRow = i;
                break;
            }
        }
        ui->presetList->blockSignals(true);
        ui->presetList->setCurrentRow(matchRow);
        ui->presetList->blockSignals(false);
        updatePreview();
    } else {
        ui->presetList->setCurrentRow(0);
    }
}

QmlEditDialog::~QmlEditDialog() {
    delete ui;
}

QString QmlEditDialog::resultCode() const {
    return ui->codeEdit->toPlainText();
}

void QmlEditDialog::onPresetSelected(int row) {
    if (row < 0 || row >= kPresetCount) return;
    QString code = loadResource(kPresets[row].resource);
    if (!code.isEmpty())
        ui->codeEdit->setPlainText(code);
    updatePreview();
}

void QmlEditDialog::onRefreshPreview() {
    updatePreview();
}

void QmlEditDialog::updatePreview() {
    QString code = ui->codeEdit->toPlainText().trimmed();
    if (code.isEmpty()) return;

    if (!m_previewSrc) {
        m_previewSrc = std::make_unique<DynamicInterfaceSource>(code, QSize(320, 180));
    } else {
        m_previewSrc->setQmlCode(code);
    }

    // Allow QML to initialize and lay out
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    m_previewSrc->nextFrame();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    m_previewSrc->nextFrame();

    if (!m_previewSrc->isReady()) {
        ui->previewLabel->setText("Init failed");
        ui->errorLabel->setText(m_previewSrc->lastError());
        return;
    }

    if (m_previewSrc->hasError()) {
        ui->errorLabel->setText(m_previewSrc->lastError());
    } else {
        ui->errorLabel->clear();
    }

    const uint8_t *data = m_previewSrc->frameData();
    QImage img(data, 320, 180, 320 * 3, QImage::Format_RGB888);
    ui->previewLabel->setPixmap(QPixmap::fromImage(img));
    ui->previewLabel->setText({});
}
