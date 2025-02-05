#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QComboBox>
#include <QFileInfo>
#include <QByteArray>
#include <QInputDialog>
#include <QMessageBox>
class FileModifier : public QWidget {
    Q_OBJECT

public:
    FileModifier(QWidget *parent = nullptr);

private slots:
    void startModification();
    void modifyFiles();
    void onTimerTimeout();

private:
    QLineEdit *maskEdit;
    QCheckBox *deleteFilesCheck;
    QLineEdit *outputPathEdit;
    QComboBox *overwriteCombo;
    QLineEdit *valueEdit;
    QTimer *timer;
    bool timerMode;
    int interval;

    void modifyFile(const QString &filePath, const QByteArray &value);
};

FileModifier::FileModifier(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    maskEdit = new QLineEdit(this);
    maskEdit->setPlaceholderText("Введите маску файла (.txt, .bin и т.д.)");
    layout->addWidget(maskEdit);

    deleteFilesCheck = new QCheckBox("Удалить входные файлы после обработки", this);
    layout->addWidget(deleteFilesCheck);

    outputPathEdit = new QLineEdit(this);
    outputPathEdit->setPlaceholderText("Путь для сохранения выходных файлов");
    layout->addWidget(outputPathEdit);

    overwriteCombo = new QComboBox(this);
    overwriteCombo->addItems({"Перезаписать", "Добавить счётчик к имени файла"});
    layout->addWidget(overwriteCombo);

    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText("Введите 8-байтное значение (в шестнадцатиричном формате)");
    layout->addWidget(valueEdit);

    QPushButton *startButton = new QPushButton("Старт", this);
    layout->addWidget(startButton);
    connect(startButton, &QPushButton::clicked, this, &FileModifier::startModification);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FileModifier::onTimerTimeout);
}

void FileModifier::startModification()
{
    bool ok;
    interval = QInputDialog::getInt(this, "Интервал опроса", "Введите интервал опроса в миллисекундах:", 5000, 1000, 60000, 100, &ok);

    if (!ok) {
        QMessageBox::warning(this, "Ошибка", "Неверный интервал");
        return;
    }
    timerMode = true;
    timer->start(interval);
}

void FileModifier::onTimerTimeout()
{
    modifyFiles();
}
void FileModifier::modifyFiles()
{
    QString mask = maskEdit->text();
    QString outputPath = outputPathEdit->text();
    QByteArray value = QByteArray::fromHex(valueEdit->text().toUtf8());

    if (value.size() != 8) {
        qDebug() << "Ошибка: Введите корректное 8-байтное значение.";
        return;
    }

    QDir dir(QDir::current());
    QStringList files = dir.entryList(QStringList() << "*" + mask, QDir::Files);

    if (files.isEmpty()) {
        qDebug() << "Нет файлов, соответствующих маске:" << mask;
        close();
    }

    for (const QString &file : files) {
        QString filePath = dir.absoluteFilePath(file);
        QFile inputFile(filePath);

        if (!inputFile.isOpen() && !inputFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Невозможно открыть файл: " << filePath;
            continue;
        }

        QByteArray fileData = inputFile.readAll();
        inputFile.close();

        // Выполнение операции mod2 для всего файла
        for (int i = 0; i < fileData.size(); ++i) {
            fileData[i] ^= value[i % value.size()]; // Применяем значение abs(по модулю)
        }

        QString outputFilePath = outputPath + "/" + file;
        if (overwriteCombo->currentText() == "Добавить счётчик к имени файла") {
            outputFilePath = outputPath + "/" + QFileInfo(file).baseName() + "_mod" + QFileInfo(file).suffix();
        }

        QFile outputFile(outputFilePath);
        if (outputFile.open(QIODevice::WriteOnly)) {
            outputFile.write(fileData);
            outputFile.close();
            qDebug() << "Изменённый файл сохранён: " << outputFilePath;
        } else {
            qDebug() << "Не удалось сохранить файл: " << outputFilePath;
        }

        if (deleteFilesCheck->isChecked()) {
            if (QFile::remove(filePath)) {
                qDebug() << "Удалён исходный: " << filePath;
            } else {
                qDebug() << "Не удалось удалить файл: " << filePath;
            }
        }
    }
}


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    FileModifier window;
    window.setWindowTitle("Модификатор файлов");
    window.resize(800, 600);
    window.show();
    return app.exec();
}

#include "main.moc"
