// nomoc.cpp
#include <QtWidgets>
#include <QtCore>
#include <QtGui>
#include <atomic>

/* ===========================
   Logger (Core-only)
   =========================== */
class Logger {
public:
    static void log(const QString &msg) {
        QFile f("mega.log");
        if (f.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream t(&f);
            t << QDateTime::currentDateTime().toString(Qt::ISODate)
              << " | " << msg << "\n";
        }
    }
};

/* ===========================
   Model (MVC)
   =========================== */
class DataModel : public QAbstractTableModel {
    Q_OBJECT
    QVector<QStringList> rows;
public:
    int rowCount(const QModelIndex &) const override { return rows.size(); }
    int columnCount(const QModelIndex &) const override { return 3; }

    QVariant data(const QModelIndex &i, int r) const override {
        if (!i.isValid() || r != Qt::DisplayRole) return {};
        if (i.row() >= rows.size() || i.column() >= rows[i.row()].size()) return {};
        return rows[i.row()][i.column()];
    }

    QVariant headerData(int s, Qt::Orientation o, int r) const override {
        if (o == Qt::Horizontal && r == Qt::DisplayRole) {
            static const QStringList headers{"ID","Name","Timestamp"};
            if (s >= 0 && s < headers.size()) return headers[s];
        }
        return {};
    }

    void insertRowData(const QStringList &r) {
        beginInsertRows({}, rows.size(), rows.size());
        rows.append(r);
        endInsertRows();
    }

    // ✅ Proper remove API
    bool removeLastRow() {
        if (rows.isEmpty())
            return false;
        const int r = rows.size() - 1;
        beginRemoveRows({}, r, r);
        rows.removeLast();
        endRemoveRows();
        return true;
    }
};

/* ===========================
   Undo Command
   =========================== */
class AddRowCommand : public QUndoCommand {
    DataModel *model;
    QStringList row;
public:
    AddRowCommand(DataModel *m, QStringList r)
        : model(m), row(std::move(r)) {}

    void redo() override {
        model->insertRowData(row);
    }

    void undo() override {
        model->removeLastRow();
    }
};

/* ===========================
   Worker Thread
   =========================== */
class Worker : public QObject {
    Q_OBJECT
    std::atomic<bool> stop{false};
public slots:
    void run() {
        for (int i=0;i<=100 && !stop;i++) {
            QThread::msleep(50);
            emit progress(i);
        }
        emit finished();
    }
    void cancel() { stop = true; }
signals:
    void progress(int);
    void finished();
};

/* ===========================
   Custom Widget
   =========================== */
class PaintWidget : public QWidget {
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::cyan);
        p.drawEllipse(rect().adjusted(10,10,-10,-10));
        p.drawText(rect(), Qt::AlignCenter, "Qt6 Base Only");
    }
};

/* ===========================
   Main Window
   =========================== */
class MainWindow : public QMainWindow {
    Q_OBJECT

    DataModel model;
    QUndoStack undo;
    QLabel *status;
    QSettings settings;

public:
    MainWindow() : settings("MegaQt","BaseOnly") {
        setWindowTitle("Mega Qt6 Base App");

        /* ---- Central MDI ---- */
        auto *mdi = new QMdiArea;
        setCentralWidget(mdi);

        auto *table = new QTableView;
        table->setModel(&model);
        mdi->addSubWindow(table)->show();

        auto *paint = new PaintWidget;
        mdi->addSubWindow(paint)->show();

        /* ---- Dock ---- */
        auto *dock = new QDockWidget("Actions");
        auto *box = new QWidget;
        auto *layout = new QVBoxLayout(box);

        auto *add = new QPushButton("Add Row");
        auto *run = new QPushButton("Run Task");
        auto *theme = new QPushButton("Toggle Dark");

        layout->addWidget(add);
        layout->addWidget(run);
        layout->addWidget(theme);

        dock->setWidget(box);
        addDockWidget(Qt::LeftDockWidgetArea, dock);

        /* ---- Status ---- */
        status = new QLabel("Idle");
        statusBar()->addWidget(status);

        /* ---- Menu ---- */
        auto *file = menuBar()->addMenu("&File");
        file->addAction("Exit", this, &QWidget::close);

        auto *edit = menuBar()->addMenu("&Edit");
        edit->addAction(undo.createUndoAction(this,"Undo"));
        edit->addAction(undo.createRedoAction(this,"Redo"));

        /* ---- Actions ---- */
        connect(add,&QPushButton::clicked,this,[=]{
            undo.push(new AddRowCommand(
                &model,
                {
                    QString::number(model.rowCount({})),
                    "Item",
                    QTime::currentTime().toString()
                }
            ));
            Logger::log("Row added");
        });

        connect(run,&QPushButton::clicked,this,[=]{
            auto *worker = new Worker;
            auto *thread = new QThread;

            worker->moveToThread(thread);
            connect(thread,&QThread::started, worker, &Worker::run);
            connect(worker,&Worker::progress,this,[=](int p){
                status->setText(QString("Progress %1%").arg(p));
            });
            connect(worker,&Worker::finished, thread, &QThread::quit);
            connect(worker,&Worker::finished, worker, &QObject::deleteLater);
            connect(thread,&QThread::finished, thread, &QObject::deleteLater);

            thread->start();
        });

        connect(theme,&QPushButton::clicked,this,[=]{
            static bool dark=false; dark=!dark;
            qApp->setStyleSheet(dark ?
                "QWidget{background:#111;color:#eee;}" : "");
        });

        restoreGeometry(settings.value("geo").toByteArray());
    }

    ~MainWindow() {
        settings.setValue("geo", saveGeometry());
    }
};

/* ===========================
   main()
   =========================== */
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    Logger::log("Started");

    MainWindow w;
    w.show();

    return app.exec();
}

/* ✅ Required for single-file AUTOMOC */
#include "nomoc.moc"
