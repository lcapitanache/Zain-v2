#include "alignedSqlQueryModel.h"

AlignedSqlQueryModel::AlignedSqlQueryModel(QObject *parent) : QSqlQueryModel(parent)
{
}

QVariant AlignedSqlQueryModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole)
    {
        switch(index.column())
        {
            case 1 ... 5:
            case 7 ... 8:
                return Qt::AlignCenter;
            case 6:
            case 9:
                return Qt::AlignLeft;
        }
    }
    return QSqlQueryModel::data(index, role);
}
