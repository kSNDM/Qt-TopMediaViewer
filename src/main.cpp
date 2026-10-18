#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSizePolicy>
#include <QPixmap>
#include <QUrl>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QFileInfo>
#include <QSet>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QSlider> // 新增：滑动条

class TopMediaWindow : public QWidget {
public:
    TopMediaWindow(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("置顶图片/视频查看器");
        resize(900, 650);

        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        isTop = true;

        // 图片显示区域
        imageLabel = new QLabel("点击“打开文件”选择图片或视频", this);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        imageLabel->setStyleSheet("background:#222; color:#fff; font-size:16px;");
        imageLabel->setMinimumSize(1, 1);

        // 视频显示区域
        videoWidget = new QVideoWidget(this);
        videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        videoWidget->setMinimumSize(1, 1);
        videoWidget->hide();

        // 视频播放器
        player = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput);
        player->setVideoOutput(videoWidget);

        connect(player, &QMediaPlayer::mediaStatusChanged,
                this, &TopMediaWindow::onMediaStatus);

        // 按钮
        btnOpen = new QPushButton("打开文件", this);
        btnPlay = new QPushButton("播放/暂停", this);
        btnTop = new QPushButton("取消置顶", this);

        connect(btnOpen, &QPushButton::clicked, this, &TopMediaWindow::openFile);
        connect(btnPlay, &QPushButton::clicked, this, &TopMediaWindow::togglePlay);
        connect(btnTop, &QPushButton::clicked, this, &TopMediaWindow::toggleTop);

        // 新增：透明度滑动条
        QLabel *opacityLabel = new QLabel("透明度:", this);
        opacitySlider = new QSlider(Qt::Horizontal, this);
        opacitySlider->setRange(10, 100); // 范围：10% 到 100%
        opacitySlider->setValue(100);     // 默认完全不透明
        opacitySlider->setFixedWidth(120); // 设置固定宽度，不要太长
        
        // 绑定滑动条信号：滑动时调整窗口透明度
        connect(opacitySlider, &QSlider::valueChanged, this, [this](int value) {
            this->setWindowOpacity(value / 100.0); // 将整数转为 0.1 ~ 1.0 的浮点数
        });

        // 布局
        auto *topLayout = new QHBoxLayout;
        topLayout->addWidget(btnOpen);
        topLayout->addWidget(btnPlay);
        topLayout->addWidget(btnTop);
        topLayout->addStretch(); // 把控件往左挤
        topLayout->addWidget(opacityLabel);
        topLayout->addWidget(opacitySlider);

        auto *layout = new QVBoxLayout(this);
        layout->addLayout(topLayout);
        layout->addWidget(imageLabel);
        layout->addWidget(videoWidget);

        currentType = None;
        zoomFactor = 1.0;
        isDragging = false;
        imageOffset = QPoint(0, 0);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if (currentType == Image && !currentPixmap.isNull()) {
            updateImage();
        }
    }

    void wheelEvent(QWheelEvent *event) override {
        if (currentType != Image || currentPixmap.isNull()) {
            QWidget::wheelEvent(event);
            return;
        }
        if (event->angleDelta().y() > 0) zoomFactor *= 1.15;
        else zoomFactor /= 1.15;
        zoomFactor = qBound(0.1, zoomFactor, 10.0);
        updateImage();
        event->accept();
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (currentType == Image && event->button() == Qt::LeftButton) {
            isDragging = true;
            lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (currentType != Image || !isDragging) {
            QWidget::mouseMoveEvent(event);
            return;
        }
        QPoint delta = event->pos() - lastMousePos;
        imageOffset += delta;
        lastMousePos = event->pos();
        updateImage();
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (currentType == Image && event->button() == Qt::LeftButton) {
            isDragging = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

private:
    enum MediaType { None, Image, Video };

    void openFile() {
        QString filter = "媒体文件 (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm);;所有文件 (*.*)";
        QString path = QFileDialog::getOpenFileName(this, "选择图片或视频", "", filter);
        if (path.isEmpty()) return;
        QString ext = "." + QFileInfo(path).suffix().toLower();

        static const QSet<QString> imageExts = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"};
        static const QSet<QString> videoExts = {".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm"};

        if (imageExts.contains(ext)) showImage(path);
        else if (videoExts.contains(ext)) showVideo(path);
        else QMessageBox::warning(this, "不支持", "暂不支持这种文件格式");
    }

    void showImage(const QString &path) {
        player->stop();
        videoWidget->hide();
        imageLabel->show();
        QPixmap pix(path);
        if (pix.isNull()) {
            QMessageBox::warning(this, "错误", "图片加载失败");
            return;
        }
        currentPixmap = pix;
        currentType = Image;
        zoomFactor = 1.0;
        imageOffset = QPoint(0, 0);
        updateImage();
    }

    void showVideo(const QString &path) {
        imageLabel->hide();
        videoWidget->show();
        currentType = Video;
        player->setSource(QUrl::fromLocalFile(path));
        player->play();
    }

    void updateImage() {
        if (currentPixmap.isNull()) return;
        QSize targetSize = imageLabel->size() * zoomFactor;
        QPixmap scaled = currentPixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmap canvas(imageLabel->size());
        canvas.fill(QColor(34, 34, 34));
        QPainter painter(&canvas);
        int x = (imageLabel->width() - scaled.width()) / 2 + imageOffset.x();
        int y = (imageLabel->height() - scaled.height()) / 2 + imageOffset.y();
        painter.drawPixmap(x, y, scaled);
        imageLabel->setPixmap(canvas);
    }

    void togglePlay() {
        if (currentType != Video) return;
        if (player->playbackState() == QMediaPlayer::PlayingState) player->pause();
        else player->play();
    }

    void toggleTop() {
        isTop = !isTop;
        setWindowFlag(Qt::WindowStaysOnTopHint, isTop);
        btnTop->setText(isTop ? "取消置顶" : "置顶");
        show();
    }

    void onMediaStatus(QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            player->setPosition(0);
            player->play();
        }
    }

    QLabel *imageLabel;
    QVideoWidget *videoWidget;
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    QPushButton *btnOpen;
    QPushButton *btnPlay;
    QPushButton *btnTop;
    QSlider *opacitySlider; // 成员变量
    QPixmap currentPixmap;
    MediaType currentType;
    bool isTop;
    double zoomFactor;
    bool isDragging;
    QPoint lastMousePos;
    QPoint imageOffset;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TopMediaWindow w;
    w.show();
    return app.exec();
}