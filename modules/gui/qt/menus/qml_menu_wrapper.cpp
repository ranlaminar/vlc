/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#include "qml_menu_wrapper.hpp"
#include "menus.hpp"
#include "util/qml_main_context.hpp"
#include "medialibrary/medialib.hpp"
#include "medialibrary/mlvideomodel.hpp"
#include "medialibrary/mlalbummodel.hpp"
#include "medialibrary/mlartistmodel.hpp"
#include "medialibrary/mlgenremodel.hpp"
#include "medialibrary/mlalbumtrackmodel.hpp"
#include "medialibrary/mlurlmodel.hpp"
#include "network/networkmediamodel.hpp"
#include "playlist/playlist_controller.hpp"
#include "playlist/playlist_model.hpp"
#include "dialogs/dialogs_provider.hpp"


#include <QSignalMapper>


static inline void addSubMenu( QMenu *func, QString title, QMenu *bar ) {
    func->setTitle( title );
    bar->addMenu( func);
}

QmlGlobalMenu::QmlGlobalMenu(QObject *parent)
    : VLCMenuBar(parent)
{
}

void QmlGlobalMenu::popup(QPoint pos)
{
    if (!m_ctx)
        return;

    intf_thread_t* p_intf = m_ctx->getIntf();
    if (!p_intf)
        return;

    QMenu* menu = new QMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QMenu* submenu;

    QAction* fileMenu = menu->addMenu(FileMenu( p_intf, menu, p_intf->p_sys->p_mi ));
    fileMenu->setText(qtr( "&Media" ));

    /* Dynamic menus, rebuilt before being showed */
    submenu = menu->addMenu(qtr( "P&layback" ));
    NavigMenu( p_intf, submenu );

    submenu = menu->addMenu(qtr( "&Audio" ));
    AudioMenu( p_intf, submenu );

    submenu = menu->addMenu(qtr( "&Video" ));
    VideoMenu( p_intf, submenu );

    submenu = menu->addMenu(qtr( "Subti&tle" ));
    SubtitleMenu( p_intf, submenu );

    submenu = menu->addMenu(qtr( "Tool&s" ));
    ToolsMenu( p_intf, submenu );

    /* View menu, a bit different */
    submenu = menu->addMenu(qtr( "V&iew" ));
    ViewMenu( p_intf, submenu, p_intf->p_sys->p_mi );

    QAction* helpMenu = menu->addMenu( HelpMenu(menu) );
    helpMenu->setText(qtr( "&Help" ));

    menu->popup(pos);
}

BaseMedialibMenu::BaseMedialibMenu(QObject* parent)
    : QObject(parent)
{}

void BaseMedialibMenu::medialibAudioContextMenu(MediaLib* ml, const QVariantList& mlId, const QPoint& pos, const QVariantMap& options)
{
    QMenu* menu = new QMenu();
    QAction* action;

    menu->setAttribute(Qt::WA_DeleteOnClose);

    action = menu->addAction( qtr("Add and play") );
    connect(action, &QAction::triggered, [ml, mlId]( ) {
        ml->addAndPlay(mlId);
    });

    action = menu->addAction( qtr("Enqueue") );
    connect(action, &QAction::triggered, [ml, mlId]( ) {
        ml->addToPlaylist(mlId);
    });

    if (options.contains("information") && options["information"].type() == QVariant::Int) {

        action = menu->addAction( qtr("Information") );
        QSignalMapper* sigmapper = new QSignalMapper(menu);
        connect(action, &QAction::triggered, sigmapper, QOverload<>::of(&QSignalMapper::map));
        sigmapper->setMapping(action, options["information"].toInt());
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
        connect(sigmapper, &QSignalMapper::mappedInt, this, &BaseMedialibMenu   ::showMediaInformation);
#else
        connect(sigmapper, QOverload<int>::of(&QSignalMapper::mapped), this, &BaseMedialibMenu::showMediaInformation);
#endif
    }
    menu->popup(pos);
}

AlbumContextMenu::AlbumContextMenu(QObject* parent)
    : BaseMedialibMenu(parent)
{}

void AlbumContextMenu::popup(const QModelIndexList& selected, QPoint pos, QVariantMap options)
{
    BaseMedialibMenu::popup(m_model, MLAlbumModel::ALBUM_ID, selected, pos, options);
}


ArtistContextMenu::ArtistContextMenu(QObject* parent)
    : BaseMedialibMenu(parent)
{}

void ArtistContextMenu::popup(const QModelIndexList &selected, QPoint pos, QVariantMap options)
{
    BaseMedialibMenu::popup(m_model, MLArtistModel::ARTIST_ID, selected, pos, options);
}

GenreContextMenu::GenreContextMenu(QObject* parent)
    : BaseMedialibMenu(parent)
{}

void GenreContextMenu::popup(const QModelIndexList& selected, QPoint pos, QVariantMap options)
{
    BaseMedialibMenu::popup(m_model, MLGenreModel::GENRE_ID, selected, pos, options);
}

AlbumTrackContextMenu::AlbumTrackContextMenu(QObject* parent)
    : BaseMedialibMenu(parent)
{}

void AlbumTrackContextMenu::popup(const QModelIndexList &selected, QPoint pos, QVariantMap options)
{
    BaseMedialibMenu::popup(m_model, MLAlbumTrackModel::TRACK_ID, selected, pos, options);
}

URLContextMenu::URLContextMenu(QObject* parent)
    : BaseMedialibMenu(parent)
{}

void URLContextMenu::popup(const QModelIndexList &selected, QPoint pos, QVariantMap options)
{
    BaseMedialibMenu::popup(m_model, MLUrlModel::URL_ID, selected, pos, options);
}


VideoContextMenu::VideoContextMenu(QObject* parent)
    : QObject(parent)
{}

void VideoContextMenu::popup(const QModelIndexList& selected, QPoint pos, QVariantMap options)
{
    if (!m_model)
        return;

    QMenu* menu = new QMenu();
    QAction* action;

    menu->setAttribute(Qt::WA_DeleteOnClose);

    MediaLib* ml= m_model->ml();

    QVariantList itemIdList;
    for (const QModelIndex& modelIndex : selected)
        itemIdList.push_back(m_model->data(modelIndex, MLVideoModel::VIDEO_ID));

    action = menu->addAction( qtr("Add and play") );

    connect(action, &QAction::triggered, [ml, itemIdList]( ) {
        ml->addAndPlay(itemIdList);
    });

    action = menu->addAction( qtr("Enqueue") );
    connect(action, &QAction::triggered, [ml, itemIdList]( ) {
        ml->addToPlaylist(itemIdList);
    });

    action = menu->addAction( qtr("Play as audio") );
    connect(action, &QAction::triggered, [ml, itemIdList]( ) {
        QStringList options({":no-video"});
        ml->addAndPlay(itemIdList, &options);
    });

    if (options.contains("information") && options["information"].type() == QVariant::Int) {
        action = menu->addAction( qtr("Information") );
        QSignalMapper* sigmapper = new QSignalMapper(menu);
        connect(action, &QAction::triggered, sigmapper, QOverload<>::of(&QSignalMapper::map));
        sigmapper->setMapping(action, options["information"].toInt());
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
        connect(sigmapper, &QSignalMapper::mappedInt, this, &VideoContextMenu::showMediaInformation);
#else
        connect(sigmapper, QOverload<int>::of(&QSignalMapper::mapped), this, &VideoContextMenu::showMediaInformation);
#endif
    }

    menu->popup(pos);
}

NetworkMediaContextMenu::NetworkMediaContextMenu(QObject* parent)
    : QObject(parent)
{}

void NetworkMediaContextMenu::popup(const QModelIndexList& selected, QPoint pos)
{
    if (!m_model)
        return;

    QMenu* menu = new QMenu();
    QAction* action;

    menu->setAttribute(Qt::WA_DeleteOnClose);

    action = menu->addAction( qtr("Add and play") );
    connect(action, &QAction::triggered, [this, selected]( ) {
        m_model->addAndPlay(selected);
    });

    action = menu->addAction( qtr("Enqueue") );
    connect(action, &QAction::triggered, [this, selected]( ) {
        m_model->addToPlaylist(selected);
    });

    bool canBeIndexed = false;
    unsigned countIndexed = 0;
    for (const QModelIndex& idx : selected)
    {
        QVariant canIndex = m_model->data(m_model->index(idx.row()), NetworkMediaModel::NETWORK_CANINDEX );
        if (canIndex.isValid() && canIndex.toBool())
            canBeIndexed = true;
        else
            continue;
        QVariant isIndexed = m_model->data(m_model->index(idx.row()), NetworkMediaModel::NETWORK_INDEXED );
        if (!isIndexed.isValid())
            continue;
        if (isIndexed.toBool())
            ++countIndexed;
    }

    if (canBeIndexed)
    {
        bool removeFromML = countIndexed > 0;
        action = menu->addAction(removeFromML
            ? qtr("Remove from Media Library")
            : qtr("Add to Media Library"));

        connect(action, &QAction::triggered, [this, selected, removeFromML]( ) {
            for (const QModelIndex& idx : selected) {
                m_model->setData(m_model->index(idx.row()), !removeFromML, NetworkMediaModel::NETWORK_INDEXED);
            }
        });
    }

    menu->popup(pos);
}

PlaylistContextMenu::PlaylistContextMenu(QObject* parent)
    : QObject(parent)
{}

void PlaylistContextMenu::popup(int currentIndex, QPoint pos )
{
    if (!m_controler || !m_model)
        return;

    QMenu* menu = new QMenu();
    QAction* action;

    QList<QUrl> selectedUrlList;
    for (const int modelIndex : m_model->getSelection())
        selectedUrlList.push_back(m_model->itemAt(modelIndex).getUrl());

    PlaylistItem currentItem;
    if (currentIndex >= 0)
        currentItem = m_model->itemAt(currentIndex);

    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (currentItem)
    {
        action = menu->addAction( qtr("Play") );
        connect(action, &QAction::triggered, [this, currentIndex]( ) {
            m_controler->goTo(currentIndex, true);
        });

        menu->addSeparator();
    }

    if (m_model->getSelectedCount() > 0) {
        action = menu->addAction( qtr("Stream") );
        connect(action, &QAction::triggered, [selectedUrlList]( ) {
            DialogsProvider::getInstance()->streamingDialog(selectedUrlList, false);
        });

        action = menu->addAction( qtr("Save") );
        connect(action, &QAction::triggered, [selectedUrlList]( ) {
            DialogsProvider::getInstance()->streamingDialog(selectedUrlList, true);
        });

        menu->addSeparator();
    }

    if (currentItem) {
        action = menu->addAction( qtr("Information") );
        action->setIcon(QIcon(":/menu/info.svg"));
        connect(action, &QAction::triggered, [currentItem]( ) {
            DialogsProvider::getInstance()->mediaInfoDialog(currentItem);
        });

        menu->addSeparator();

        action = menu->addAction( qtr("Show Containing Directory...") );
        action->setIcon(QIcon(":/type/folder-grey.svg"));
        connect(action, &QAction::triggered, [currentItem]( ) {
            DialogsProvider::getInstance()->mediaInfoDialog(currentItem);
        });

        menu->addSeparator();
    }

    action = menu->addAction( qtr("Add File...") );
    action->setIcon(QIcon(":/buttons/playlist/playlist_add.svg"));
    connect(action, &QAction::triggered, []( ) {
        DialogsProvider::getInstance()->simpleOpenDialog(false);
    });

    action = menu->addAction( qtr("Add Directory...") );
    action->setIcon(QIcon(":/buttons/playlist/playlist_add.svg"));
    connect(action, &QAction::triggered, []( ) {
        DialogsProvider::getInstance()->PLAppendDir();
    });

    action = menu->addAction( qtr("Advanced Open...") );
    action->setIcon(QIcon(":/buttons/playlist/playlist_add.svg"));
    connect(action, &QAction::triggered, []( ) {
        DialogsProvider::getInstance()->PLAppendDialog();
    });

    menu->addSeparator();

    if (m_model->getSelectedCount() > 0)
    {
        action = menu->addAction( qtr("Save Playlist to File...") );
        connect(action, &QAction::triggered, []( ) {
            DialogsProvider::getInstance()->savePlayingToPlaylist();
        });

        menu->addSeparator();

        action = menu->addAction( qtr("Remove Selected") );
        action->setIcon(QIcon(":/buttons/playlist/playlist_remove.svg"));
        connect(action, &QAction::triggered, [this]( ) {
            m_model->removeItems(m_model->getSelection());
        });
    }

    action = menu->addAction( qtr("Clear the playlist") );
    action->setIcon(QIcon(":/toolbar/clear.svg"));
    connect(action, &QAction::triggered, [this]( ) {
        m_controler->clear();
    });

    menu->popup(pos);
}
