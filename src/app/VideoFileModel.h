#ifndef VIDEOFILEMODEL_H
#define VIDEOFILEMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct VideoFileItem {
    QString path;
    QString fileName;
    QString folder;
    QString fixedPath;
    QString sourcePreviewPath;
    QString fixedPreviewPath;
    QString thumbnailUrl;
    QString fixedThumbnailUrl;
    QString resolution = "?";
    QString frames = "?F";
    QString fps = "?";
    bool fixes = false;
    bool srcPreview = false;
    bool srcTelegram = false;
    bool fixedPreview = false;
    bool fixedTelegram = false;
    bool fixedExists = false;
    bool sourcePreviewExists = false;
    bool fixedPreviewExists = false;
    QString runtimeStatus;
};

class VideoFileModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        PathRole = Qt::UserRole + 1,
        FileNameRole,
        FolderRole,
        FixedPathRole,
        SourcePreviewPathRole,
        FixedPreviewPathRole,
        FixesRole,
        SrcPreviewRole,
        SrcTelegramRole,
        FixedPreviewRole,
        FixedTelegramRole,
        FixedExistsRole,
        SourcePreviewExistsRole,
        FixedPreviewExistsRole,
        StatusRole,
        ResolutionRole,
        FramesRole,
        FpsRole,
        ThumbnailUrlRole,
        FixedThumbnailUrlRole
    };
    Q_ENUM(Role)

    explicit VideoFileModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void setAction(int row, const QString& action, bool enabled);

    void setFiles(const QStringList& files);
    QStringList addFiles(const QStringList& files);
    QStringList paths() const;
    QStringList filesWithAction(const QString& action, QVector<int>* rows = nullptr) const;
    bool actionAt(int row, const QString& action) const;
    QString fileAt(int row) const;
    QString fixedPathAt(int row) const;
    QString sourcePreviewPathAt(int row) const;
    QString fixedPreviewPathAt(int row) const;
    int rowForPath(const QString& path) const;
    int rowForFixedPath(const QString& path) const;
    int rowForPreviewPath(const QString& path) const;
    void setRuntimeStatus(int row, const QString& status);
    void clearRuntimeStatuses();
    void refreshExistence();
    void setMetadata(int row, const QString& resolution, const QString& frames, const QString& fps);
    void setThumbnailUrl(int row, const QString& thumbnailUrl);
    void setFixedThumbnailUrl(int row, const QString& thumbnailUrl);

    static bool isVideoPath(const QString& path);
    static QString makeFixedPath(const QString& input);
    static QString makePreviewPath(const QString& input);

signals:
    void countChanged();

private:
    void updateDerivedPaths(int row);
    void refreshExistence(int row);
    QString statusText(const VideoFileItem& item) const;

private:
    QVector<VideoFileItem> m_items;
};

#endif // VIDEOFILEMODEL_H
