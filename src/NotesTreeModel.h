#ifndef NOTESTREEMODEL_H
#define NOTESTREEMODEL_H

#include "Note.h"

class NotesTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	enum Roles {
			NameRole = Qt::UserRole + 1,
	};

	explicit NotesTreeModel(QObject *parent = nullptr);

	// QAbstractItemModel interface
	//QMap<int, QVariant> itemData(const QModelIndex &index) const override;
	//bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
	QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	//bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
	//QHash<int,QByteArray> roleNames() const override;
	bool insertRows(int row, int count, const QModelIndex &parent) override;
	//bool insertColumns(int column, int count, const QModelIndex &parent) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QModelIndex index(int row, int column,
										const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

signals:
	void modelChanged();

public slots:
	void rootPath(QString path);

private:
	Note root;

};

#endif // NOTESTREEMODEL_H
