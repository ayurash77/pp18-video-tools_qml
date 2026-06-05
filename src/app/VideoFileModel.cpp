#include "VideoFileModel.h"

#include "services/HandbrakePreviewService.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QSet>

#include <utility>

namespace {
const QSet<QString> kVideoExtensions{
    "3g2", "3gp", "avi", "dv", "flv", "m2ts", "m4v", "mkv", "mov", "mp4",
    "mpeg", "mpg", "mts", "mxf", "ogv", "ts", "vob", "webm", "wmv"
};

VideoFileItem makeVideoFileItem(const QString& path)
{
    const QFileInfo fileInfo(path);

    VideoFileItem item;
    item.path = fileInfo.absoluteFilePath();
    item.fileName = fileInfo.fileName();
    item.folder = QDir::toNativeSeparators(fileInfo.absolutePath());
    item.fixedPath = VideoFileModel::makeFixedPath(item.path);
    item.fixes = false;
    item.srcPreview = true;
    item.srcTelegram = true;
    item.fixedPreview = false;
    item.fixedTelegram = false;
    item.sourcePreviewPath = VideoFileModel::makePreviewPath(item.path);
    item.fixedPreviewPath = VideoFileModel::makePreviewPath(item.fixedPath);
    return item;
}
}

VideoFileModel::VideoFileModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int VideoFileModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant VideoFileModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const VideoFileItem& item = m_items.at(index.row());
    switch (role) {
    case PathRole:
        return item.path;
    case FileNameRole:
        return item.fileName;
    case FolderRole:
        return item.folder;
    case FixedPathRole:
        return item.fixedPath;
    case SourcePreviewPathRole:
        return item.sourcePreviewPath;
    case FixedPreviewPathRole:
        return item.fixedPreviewPath;
    case ThumbnailUrlRole:
        return item.thumbnailUrl;
    case FixedThumbnailUrlRole:
        return item.fixedThumbnailUrl;
    case FixesRole:
        return item.fixes;
    case SrcPreviewRole:
        return item.srcPreview;
    case SrcTelegramRole:
        return item.srcTelegram;
    case FixedPreviewRole:
        return item.fixedPreview;
    case FixedTelegramRole:
        return item.fixedTelegram;
    case FixedExistsRole:
        return item.fixedExists;
    case SourcePreviewExistsRole:
        return item.sourcePreviewExists;
    case FixedPreviewExistsRole:
        return item.fixedPreviewExists;
    case StatusRole:
        return statusText(item);
    case ResolutionRole:
        return item.resolution;
    case FramesRole:
        return item.frames;
    case FpsRole:
        return item.fps;
    default:
        return {};
    }
}

bool VideoFileModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;

    VideoFileItem& item = m_items[index.row()];
    bool changed = false;
    switch (role) {
    case FixesRole:
        changed = item.fixes != value.toBool();
        item.fixes = value.toBool();
        break;
    case SrcPreviewRole:
        changed = item.srcPreview != value.toBool();
        item.srcPreview = value.toBool();
        break;
    case SrcTelegramRole:
        changed = item.srcTelegram != value.toBool();
        item.srcTelegram = value.toBool();
        break;
    case FixedPreviewRole:
        changed = item.fixedPreview != value.toBool();
        item.fixedPreview = value.toBool();
        break;
    case FixedTelegramRole:
        changed = item.fixedTelegram != value.toBool();
        item.fixedTelegram = value.toBool();
        break;
    default:
        return false;
    }

    if (!changed)
        return false;

    if (role == FixesRole) {
        updateDerivedPaths(index.row());
        refreshExistence(index.row());
        emit dataChanged(index, index, {role, FixedPathRole, SourcePreviewPathRole, FixedPreviewPathRole,
                         FixedExistsRole, SourcePreviewExistsRole, FixedPreviewExistsRole, StatusRole});
        return true;
    }

    emit dataChanged(index, index, {role, StatusRole});
    return true;
}

Qt::ItemFlags VideoFileModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> VideoFileModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {FileNameRole, "fileName"},
        {FolderRole, "folder"},
        {FixedPathRole, "fixedPath"},
        {SourcePreviewPathRole, "sourcePreviewPath"},
        {FixedPreviewPathRole, "fixedPreviewPath"},
        {ThumbnailUrlRole, "thumbnailUrl"},
        {FixedThumbnailUrlRole, "fixedThumbnailUrl"},
        {FixesRole, "fixes"},
        {SrcPreviewRole, "srcPreview"},
        {SrcTelegramRole, "srcTelegram"},
        {FixedPreviewRole, "fixedPreview"},
        {FixedTelegramRole, "fixedTelegram"},
        {FixedExistsRole, "fixedExists"},
        {SourcePreviewExistsRole, "sourcePreviewExists"},
        {FixedPreviewExistsRole, "fixedPreviewExists"},
        {StatusRole, "status"},
        {ResolutionRole, "resolution"},
        {FramesRole, "frames"},
        {FpsRole, "fps"}
    };
}

int VideoFileModel::count() const
{
    return m_items.size();
}

void VideoFileModel::clear()
{
    if (m_items.isEmpty())
        return;

    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

void VideoFileModel::removeAt(int row)
{
    if (row < 0 || row >= m_items.size())
        return;

    beginRemoveRows({}, row, row);
    m_items.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void VideoFileModel::setAction(int row, const QString& action, bool enabled)
{
    if (row < 0 || row >= m_items.size())
        return;

    int role = 0;
    if (action == "fixes")
        role = FixesRole;
    else if (action == "srcPreview")
        role = SrcPreviewRole;
    else if (action == "srcTelegram")
        role = SrcTelegramRole;
    else if (action == "fixedPreview")
        role = FixedPreviewRole;
    else if (action == "fixedTelegram")
        role = FixedTelegramRole;
    else if (action == "preview")
        role = FixedPreviewRole;
    else if (action == "telegram")
        role = FixedTelegramRole;
    else
        return;

    setData(index(row), enabled, role);
}

void VideoFileModel::setFiles(const QStringList& files)
{
    QHash<QString, VideoFileItem> existingByPath;
    existingByPath.reserve(m_items.size());
    for (const VideoFileItem& item : std::as_const(m_items))
        existingByPath.insert(item.path, item);

    QVector<VideoFileItem> nextItems;
    QSet<QString> seen;
    nextItems.reserve(files.size());

    for (const QString& rawPath : files) {
        const QFileInfo fileInfo(rawPath);
        const QString path = fileInfo.absoluteFilePath();
        if (!isVideoPath(path) || seen.contains(path))
            continue;

        seen.insert(path);

        VideoFileItem item = makeVideoFileItem(path);
        const auto existing = existingByPath.constFind(path);
        if (existing != existingByPath.cend()) {
            item.fixes = existing->fixes;
            item.srcPreview = existing->srcPreview;
            item.srcTelegram = existing->srcTelegram;
            item.fixedPreview = existing->fixedPreview;
            item.fixedTelegram = existing->fixedTelegram;
            item.resolution = existing->resolution;
            item.frames = existing->frames;
            item.fps = existing->fps;
            item.thumbnailUrl = existing->thumbnailUrl;
            item.fixedThumbnailUrl = existing->fixedThumbnailUrl;
        }

        nextItems << item;
    }

    beginResetModel();
    m_items = nextItems;
    for (int row = 0; row < m_items.size(); ++row)
        refreshExistence(row);
    endResetModel();
    emit countChanged();
}

QStringList VideoFileModel::addFiles(const QStringList& files)
{
    QSet<QString> seen;
    for (const VideoFileItem& item : std::as_const(m_items))
        seen.insert(item.path);

    QVector<VideoFileItem> additions;
    QStringList addedFiles;
    additions.reserve(files.size());

    for (const QString& rawPath : files) {
        const QFileInfo fileInfo(rawPath);
        const QString path = fileInfo.absoluteFilePath();
        if (!isVideoPath(path) || seen.contains(path))
            continue;

        seen.insert(path);
        additions << makeVideoFileItem(path);
        addedFiles << path;
    }

    if (additions.isEmpty())
        return {};

    const int first = m_items.size();
    const int last = first + additions.size() - 1;
    beginInsertRows({}, first, last);
    for (const VideoFileItem& item : additions)
        m_items << item;
    for (int row = first; row <= last; ++row)
        refreshExistence(row);
    endInsertRows();
    emit countChanged();
    return addedFiles;
}

QStringList VideoFileModel::paths() const
{
    QStringList result;
    result.reserve(m_items.size());
    for (const VideoFileItem& item : m_items)
        result << item.path;
    return result;
}

QStringList VideoFileModel::filesWithAction(const QString& action, QVector<int>* rows) const
{
    QStringList files;
    if (rows)
        rows->clear();

    for (int row = 0; row < m_items.size(); ++row) {
        const VideoFileItem& item = m_items.at(row);
        if ((action == "fixes" && item.fixes)
            || (action == "srcPreview" && item.srcPreview)
            || (action == "srcTelegram" && item.srcTelegram)
            || (action == "fixedPreview" && item.fixedPreview)
            || (action == "fixedTelegram" && item.fixedTelegram)
            || (action == "preview" && (item.srcPreview || item.fixedPreview))
            || (action == "telegram" && (item.srcTelegram || item.fixedTelegram))) {
            files << item.path;
            if (rows)
                rows->append(row);
        }
    }
    return files;
}

bool VideoFileModel::actionAt(int row, const QString& action) const
{
    if (row < 0 || row >= m_items.size())
        return false;

    const VideoFileItem& item = m_items.at(row);
    if (action == "fixes")
        return item.fixes;
    if (action == "srcPreview")
        return item.srcPreview;
    if (action == "srcTelegram")
        return item.srcTelegram;
    if (action == "fixedPreview")
        return item.fixedPreview;
    if (action == "fixedTelegram")
        return item.fixedTelegram;
    if (action == "preview")
        return item.srcPreview || item.fixedPreview;
    if (action == "telegram")
        return item.srcTelegram || item.fixedTelegram;
    return false;
}

QString VideoFileModel::fileAt(int row) const
{
    return row >= 0 && row < m_items.size() ? m_items.at(row).path : QString();
}

QString VideoFileModel::fixedPathAt(int row) const
{
    return row >= 0 && row < m_items.size() ? m_items.at(row).fixedPath : QString();
}

QString VideoFileModel::sourcePreviewPathAt(int row) const
{
    return row >= 0 && row < m_items.size() ? m_items.at(row).sourcePreviewPath : QString();
}

QString VideoFileModel::fixedPreviewPathAt(int row) const
{
    return row >= 0 && row < m_items.size() ? m_items.at(row).fixedPreviewPath : QString();
}

int VideoFileModel::rowForPath(const QString& path) const
{
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items.at(row).path == path)
            return row;
    }
    return -1;
}

int VideoFileModel::rowForFixedPath(const QString& path) const
{
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items.at(row).fixedPath == path)
            return row;
    }
    return -1;
}

int VideoFileModel::rowForPreviewPath(const QString& path) const
{
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items.at(row).sourcePreviewPath == path || m_items.at(row).fixedPreviewPath == path)
            return row;
    }
    return -1;
}

void VideoFileModel::setRuntimeStatus(int row, const QString& status)
{
    if (row < 0 || row >= m_items.size())
        return;
    if (m_items[row].runtimeStatus == status)
        return;

    m_items[row].runtimeStatus = status;
    const QModelIndex changedIndex = index(row);
    emit dataChanged(changedIndex, changedIndex, {StatusRole});
}

void VideoFileModel::clearRuntimeStatuses()
{
    if (m_items.isEmpty())
        return;

    for (VideoFileItem& item : m_items)
        item.runtimeStatus.clear();

    emit dataChanged(index(0), index(m_items.size() - 1), {StatusRole});
}

void VideoFileModel::refreshExistence()
{
    if (m_items.isEmpty())
        return;

    for (int row = 0; row < m_items.size(); ++row)
        refreshExistence(row);

    emit dataChanged(index(0), index(m_items.size() - 1), {FixedExistsRole, SourcePreviewExistsRole, FixedPreviewExistsRole});
}

void VideoFileModel::setMetadata(int row, const QString& resolution, const QString& frames, const QString& fps)
{
    if (row < 0 || row >= m_items.size())
        return;

    VideoFileItem& item = m_items[row];
    item.resolution = resolution;
    item.frames = frames;
    item.fps = fps;

    const QModelIndex changedIndex = index(row);
    emit dataChanged(changedIndex, changedIndex, {ResolutionRole, FramesRole, FpsRole});
}

void VideoFileModel::setThumbnailUrl(int row, const QString& thumbnailUrl)
{
    if (row < 0 || row >= m_items.size())
        return;
    if (m_items[row].thumbnailUrl == thumbnailUrl)
        return;

    m_items[row].thumbnailUrl = thumbnailUrl;
    const QModelIndex changedIndex = index(row);
    emit dataChanged(changedIndex, changedIndex, {ThumbnailUrlRole});
}

void VideoFileModel::setFixedThumbnailUrl(int row, const QString& thumbnailUrl)
{
    if (row < 0 || row >= m_items.size())
        return;
    if (m_items[row].fixedThumbnailUrl == thumbnailUrl)
        return;

    m_items[row].fixedThumbnailUrl = thumbnailUrl;
    const QModelIndex changedIndex = index(row);
    emit dataChanged(changedIndex, changedIndex, {FixedThumbnailUrlRole});
}

bool VideoFileModel::isVideoPath(const QString& path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.isFile())
        return false;
    if (fileInfo.completeBaseName().endsWith("__preview", Qt::CaseInsensitive))
        return false;
    return kVideoExtensions.contains(fileInfo.suffix().toLower());
}

QString VideoFileModel::makeFixedPath(const QString& input)
{
    const QFileInfo fileInfo(input);
    return fileInfo.dir().absoluteFilePath("fixed/" + fileInfo.completeBaseName() + ".mov");
}

QString VideoFileModel::makePreviewPath(const QString& input)
{
    return HandbrakePreviewService::makeOutputPath(input);
}

void VideoFileModel::updateDerivedPaths(int row)
{
    if (row < 0 || row >= m_items.size())
        return;

    VideoFileItem& item = m_items[row];
    item.fixedPath = makeFixedPath(item.path);
    item.sourcePreviewPath = makePreviewPath(item.path);
    item.fixedPreviewPath = makePreviewPath(item.fixedPath);
}

void VideoFileModel::refreshExistence(int row)
{
    VideoFileItem& item = m_items[row];
    item.fixedExists = QFileInfo::exists(item.fixedPath);
    item.sourcePreviewExists = QFileInfo::exists(item.sourcePreviewPath);
    item.fixedPreviewExists = QFileInfo::exists(item.fixedPreviewPath);
}

QString VideoFileModel::statusText(const VideoFileItem& item) const
{
    if (!item.runtimeStatus.isEmpty())
        return item.runtimeStatus;

    QStringList lines;
    if (item.fixes)
        lines << "Fixes";
    if (item.srcPreview)
        lines << "SRC Preview";
    if (item.srcTelegram)
        lines << "SRC TG";
    if (item.fixedPreview)
        lines << "FIX Preview";
    if (item.fixedTelegram)
        lines << "FIX TG";
    return lines.join('\n');
}
