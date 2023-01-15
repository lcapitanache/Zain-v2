#ifndef YOURSQLQUERYMODEL_H
#define YOURSQLQUERYMODEL_H

#include <QSqlQueryModel>

class AlignedSqlQueryModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    AlignedSqlQueryModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role) const override;
};

#endif // YOURSQLQUERYMODEL_H
