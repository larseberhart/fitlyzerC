// SPDX-License-Identifier: GPL-3


#include "DatabaseConnectionGuard.h"

DatabaseConnectionGuard::DatabaseConnectionGuard(const QString& connectionName)
    : m_connectionName(connectionName)
{
    // Create the connection
    if (!QSqlDatabase::contains(connectionName))
    {
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    }
}

DatabaseConnectionGuard::~DatabaseConnectionGuard()
{
    // Ensure connection is closed before removal
    if (QSqlDatabase::contains(m_connectionName))
    {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
            if (db.isOpen())
            {
                db.close();
            }
        }
        // Remove the connection from Qt's registry
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

QSqlDatabase DatabaseConnectionGuard::database() const
{
    return QSqlDatabase::database(m_connectionName, false);
}