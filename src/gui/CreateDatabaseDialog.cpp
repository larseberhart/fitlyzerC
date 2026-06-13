#include "CreateDatabaseDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDir>

CreateDatabaseDialog::CreateDatabaseDialog(const QString& defaultDirectory, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create Database");
    setMinimumWidth(520);

    auto* mainLayout = new QVBoxLayout(this);

    auto* intro = new QLabel(
        "Create a new training database and optionally add a default athlete.", this);
    intro->setWordWrap(true);
    mainLayout->addWidget(intro);

    auto* form = new QFormLayout;
    m_nameEdit = new QLineEdit("fitlyzer", this);
    m_locationEdit = new QLineEdit(defaultDirectory, this);
    m_browseButton = new QPushButton("Browse...", this);

    auto* locationRow = new QHBoxLayout;
    locationRow->addWidget(m_locationEdit, 1);
    locationRow->addWidget(m_browseButton);

    form->addRow("Database name:", m_nameEdit);
    form->addRow("Location:", locationRow);
    mainLayout->addLayout(form);

    m_createAthleteCheck = new QCheckBox("Create default athlete", this);
    mainLayout->addWidget(m_createAthleteCheck);

    auto* athleteForm = new QFormLayout;
    m_firstNameEdit = new QLineEdit(this);
    m_lastNameEdit = new QLineEdit(this);
    m_firstNameEdit->setPlaceholderText("First name");
    m_lastNameEdit->setPlaceholderText("Last name");
    athleteForm->addRow("First name:", m_firstNameEdit);
    athleteForm->addRow("Last name:", m_lastNameEdit);
    mainLayout->addLayout(athleteForm);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_browseButton, &QPushButton::clicked, this, &CreateDatabaseDialog::browseLocation);
    connect(m_createAthleteCheck, &QCheckBox::toggled, this, &CreateDatabaseDialog::updateAthleteFields);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &CreateDatabaseDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateAthleteFields();
}

QString CreateDatabaseDialog::databasePath() const
{
    const QString dirPath = m_locationEdit ? m_locationEdit->text().trimmed() : QString();
    if (dirPath.isEmpty())
        return QString();

    return QDir(dirPath).filePath(normalizedDatabaseName());
}

bool CreateDatabaseDialog::shouldCreateDefaultAthlete() const
{
    return m_createAthleteCheck && m_createAthleteCheck->isChecked();
}

QString CreateDatabaseDialog::defaultAthleteFirstName() const
{
    return m_firstNameEdit ? m_firstNameEdit->text().trimmed() : QString();
}

QString CreateDatabaseDialog::defaultAthleteLastName() const
{
    return m_lastNameEdit ? m_lastNameEdit->text().trimmed() : QString();
}

void CreateDatabaseDialog::browseLocation()
{
    const QString current = m_locationEdit ? m_locationEdit->text().trimmed() : QString();
    const QString dir = QFileDialog::getExistingDirectory(this, "Choose Database Location", current);
    if (!dir.isEmpty() && m_locationEdit)
        m_locationEdit->setText(dir);
}

void CreateDatabaseDialog::updateAthleteFields()
{
    const bool enabled = shouldCreateDefaultAthlete();
    if (m_firstNameEdit)
        m_firstNameEdit->setEnabled(enabled);
    if (m_lastNameEdit)
        m_lastNameEdit->setEnabled(enabled);
}

void CreateDatabaseDialog::accept()
{
    const QString dirPath = m_locationEdit ? m_locationEdit->text().trimmed() : QString();
    if (dirPath.isEmpty())
    {
        QMessageBox::warning(this, "Missing Location", "Choose a folder for the new database.");
        return;
    }

    const QFileInfo dirInfo(dirPath);
    if (!dirInfo.exists() || !dirInfo.isDir())
    {
        QMessageBox::warning(this, "Invalid Location", "Choose an existing folder for the new database.");
        return;
    }

    if (normalizedDatabaseName().isEmpty())
    {
        QMessageBox::warning(this, "Missing Name", "Enter a database name.");
        return;
    }

    if (shouldCreateDefaultAthlete())
    {
        if (defaultAthleteFirstName().isEmpty() || defaultAthleteLastName().isEmpty())
        {
            QMessageBox::warning(this, "Missing Athlete Name",
                "Enter a first and last name for the default athlete, or disable that option.");
            return;
        }
    }

    QDialog::accept();
}

QString CreateDatabaseDialog::normalizedDatabaseName() const
{
    QString name = m_nameEdit ? m_nameEdit->text().trimmed() : QString();
    if (name.endsWith(".fitlyzerdb", Qt::CaseInsensitive))
        return name;
    if (!name.isEmpty())
        name += ".fitlyzerdb";
    return name;
}