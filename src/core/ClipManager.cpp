#include "core/ClipManager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

ClipManager::ClipManager() = default;

void ClipManager::appendFromFolder(const QString &folderPath) {
    QDir dir(folderPath);
    if (!dir.exists()) { qWarning() << "Folder does not exist:" << folderPath; return; }

    QStringList filters = {"*.mp4","*.avi","*.mov","*.mkv","*.webm",
                           "*.png","*.jpg","*.jpeg","*.bmp","*.webp","*.gif"};
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    for (const QString &file : dir.entryList()) {
        QString path = dir.absoluteFilePath(file);
        if (!clips.contains(path) && isMediaFile(file))
            clips.append(path);
    }
}

void ClipManager::loadFolder(const QString &folderPath) {
    clips.clear();
    appendFromFolder(folderPath);
    std::sort(clips.begin(), clips.end());
    qDebug() << "Loaded" << clips.count() << "clips from" << folderPath;
}

void ClipManager::addFolder(const QString &folderPath) {
    int before = clips.count();
    appendFromFolder(folderPath);
    std::sort(clips.begin(), clips.end());
    qDebug() << "Added" << (clips.count() - before) << "clips from" << folderPath
             << "(total:" << clips.count() << ")";
}

void ClipManager::addFiles(const QStringList &filePaths) {
    int added = 0;
    for (const QString &path : filePaths) {
        if (isMediaFile(path) && !clips.contains(path)) {
            clips.append(path);
            ++added;
        }
    }
    qDebug() << "Added" << added << "files (total:" << clips.count() << ")";
}

QString ClipManager::getClipPath(int index) const {
    return (index >= 0 && index < clips.count()) ? clips.at(index) : QString();
}

bool ClipManager::isMediaFile(const QString &path) const {
    QString l = path.toLower();
    return l.endsWith(".mp4") || l.endsWith(".avi")  || l.endsWith(".mov")  ||
           l.endsWith(".mkv") || l.endsWith(".webm") || l.endsWith(".png")  ||
           l.endsWith(".jpg") || l.endsWith(".jpeg") || l.endsWith(".bmp")  ||
           l.endsWith(".webp") || l.endsWith(".gif");
}
