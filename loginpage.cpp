#include "loginpage.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMessageBox>
#include <QRegularExpression>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
{
    // Создание вертикального компоновщика для размещения элементов интерфейса
    auto layout = new QVBoxLayout(this);
    // Добавление метки с инструкцией для пользователя
    layout->addWidget(new QLabel("Введите фамилию и имя:"));

    // Создание поля для ввода имени пользователя
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Фамилия и Имя");

    // Установка фильтра событий для контроля и валидации вводимого текста
    m_nameEdit->installEventFilter(this);

    // Добавление поля ввода в компоновку
    layout->addWidget(m_nameEdit);

    // Создание кнопок "Далее" и "Отмена"
    auto nextBtn = new QPushButton("Далее");
    auto cancelBtn = new QPushButton("Отмена");
    layout->addWidget(nextBtn);
    layout->addWidget(cancelBtn);

    // Подключение сигналов нажатия кнопок к соответствующим слотам
    connect(nextBtn, &QPushButton::clicked, this, &LoginPage::onNextClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &LoginPage::onCancelClicked);
}

QString LoginPage::userName() const
{
    // Возврат сохраненного имени пользователя
    return m_storedName;
}

void LoginPage::onNextClicked()
{
    // Получение текста из поля ввода с удалением начальных и конечных пробелов
    QString name = m_nameEdit->text().trimmed();

    // Проверка на пустой ввод
    if (name.isEmpty()) {
        // Показ предупреждающего сообщения, если поле пустое
        QMessageBox::warning(this, "Пустой ввод", "Пожалуйста, введите фамилию и имя.");
        return; // Прерывание выполнения функции
    }

    // Сохранение имени в переменную-член класса
    m_storedName = name;
    // Отправка сигнала с именем пользователя для дальнейшей обработки
    emit nextRequested(m_storedName);
}

void LoginPage::onCancelClicked()
{
    // Отправка сигнала отмены операции
    emit cancelRequested();
}

void LoginPage::clearFields() {
    // Очистка поля ввода имени
    m_nameEdit->clear();
}

bool LoginPage::eventFilter(QObject *obj, QEvent *event)
{
    // Проверка, что событие относится к полю ввода имени и является нажатием клавиши
    if (obj == m_nameEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        QString currentText = m_nameEdit->text();
        int cursorPos = m_nameEdit->cursorPosition();

        // Разрешаем управляющие клавиши (навигация, удаление, Enter)
        if (keyEvent->key() == Qt::Key_Backspace ||
            keyEvent->key() == Qt::Key_Delete ||
            keyEvent->key() == Qt::Key_Left ||
            keyEvent->key() == Qt::Key_Right ||
            keyEvent->key() == Qt::Key_Home ||
            keyEvent->key() == Qt::Key_End ||
            keyEvent->key() == Qt::Key_Enter ||
            keyEvent->key() == Qt::Key_Return) {
            return QWidget::eventFilter(obj, event); // Пропускаем событие дальше
        }

        QString inputChar = keyEvent->text();

        // Если символ пустой (например, при нажатии Shift, Ctrl и т.д.)
        if (inputChar.isEmpty()) {
            return QWidget::eventFilter(obj, event);
        }

        // Проверяем разрешен ли символ (только кириллица, пробел, дефис)
        QRegularExpression allowedChar("[А-Яа-яЁё\\- ]");
        if (!allowedChar.match(inputChar).hasMatch()) {
            return true; // Блокируем ввод неразрешенных символов
        }

        // Проверка на пробел
        if (inputChar == " ") {
            // Проверяем можно ли вводить пробел:
            // - не больше одного пробела
            // - пробел не может быть первым символом
            int spaceCount = currentText.count(' ');
            if (spaceCount >= 1 || cursorPos == 0) {
                return true; // Блокируем пробел
            }
            return QWidget::eventFilter(obj, event);
        }

        // Проверка на дефис
        if (inputChar == "-") {
            // Дефис нельзя ставить в начале строки
            if (cursorPos == 0) {
                return true;
            }

            // Дефис нельзя ставить после пробела или другого дефиса
            if (cursorPos > 0) {
                QChar before = currentText.at(cursorPos - 1);
                if (before == ' ' || before == '-') {
                    return true;
                }
            }

            // Дефис нельзя ставить перед пробелом
            if (cursorPos < currentText.length()) {
                QChar after = currentText.at(cursorPos);
                if (after == ' ') {
                    return true;
                }
            }

            // Находим текущее слово для проверки количества дефисов
            // Поиск начала слова (до пробела или начала строки)
            int wordStart = cursorPos;
            while (wordStart > 0 && currentText.at(wordStart - 1) != ' ') {
                wordStart--;
            }

            // Поиск конца слова (до пробела или конца строки)
            int wordEnd = cursorPos;
            while (wordEnd < currentText.length() && currentText.at(wordEnd) != ' ') {
                wordEnd++;
            }

            // Извлечение текущего слова
            QString currentWord = currentText.mid(wordStart, wordEnd - wordStart);

            // Проверяем количество дефисов в текущем слове
            if (currentWord.count('-') >= 1) {
                return true; // Уже есть дефис в слове, блокируем второй
            }

            return QWidget::eventFilter(obj, event);
        }

        // Если вводится кириллическая буква - разрешаем ввод
        return QWidget::eventFilter(obj, event);
    }

    // Для всех других объектов и событий передаем обработку родительскому классу
    return QWidget::eventFilter(obj, event);
}
