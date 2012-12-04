/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "gmdefs.h"
#include "gmutils.h"
#include "GMApp.h"
#include "GMTrack.h"
#include "GMList.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTrackView.h"
#include "GMTaskManager.h"
#include "GMSource.h"
#include "GMSourceView.h"
#include "GMClipboard.h"
#include "GMPodcastSource.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMIconTheme.h"
#include "GMFilename.h"
#include "GMAlbumList.h"
#include "GMAudioPlayer.h"







#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>

/*
  Notes:


    PodcastDownloader
      - don't update db, but send callback to mainthread to update status
*/

FXString gm_rfc1123(FXTime time) {
  FXDate date;
  date.setTime(time);
  FXString str;
  str  = FXString::value("%s, %d %s %d ",FXDate::dayNameShort(date.dayOfWeek()),date.day(),FXDate::monthNameShort(date.month()),date.year());
  str += FXSystem::universalTime("%T GMT",time);
  return str;
  }


//-----------------------------------------------------------------------------


struct FeedLink {
  FXString description;
  FXString url;
  };

class HtmlFeedParser : public HtmlParser{
public:
  enum {
    Elem_Html = XmlParser::Elem_Last,
    Elem_Head,
    Elem_Meta,
    };
protected:
  FXint begin(const FXchar * element,const FXchar** attributes) {
    //fxmessage("s %d: %s\n",node(),element);
    switch(node()) {
      case Elem_None:
        if (comparecase(element,"html")==0)
          return Elem_Html;
        break;
      case Elem_Html:
        if (comparecase(element,"head")==0)
          return Elem_Head;
        break;
      case Elem_Head:
        if (comparecase(element,"link")==0)
          parse_link(attributes);
        break;
      };
    return Elem_Skip;
    }
  //void data(const FXuchar*,FXint){
  //  }

  //void end(const FXchar*) {
  //  fxmessage("e %d: %s\n",node(),element);
  //  }

protected:
  void parse_link(const FXchar ** attributes) {
    if (attributes) {
      FXbool valid=false;
      FeedLink feed;
      for (FXint i=0;attributes[i];i+=2) {
        //GM_DEBUG_PRINT("%s=%s\n",attributes[i],attributes[i+1]);
        if (comparecase(attributes[i],"href")==0)
          feed.url = attributes[i+1];
        else if (comparecase(attributes[i],"type")==0)
          if (comparecase(attributes[i+1],"application/rss+xml")==0)
            valid=true;
          else
            return;
        else if (comparecase(attributes[i],"title")==0)
          feed.description = attributes[i+1];
        }
      if (valid) {
        links.append(feed);
        GM_DEBUG_PRINT("%s\n%s\n",feed.description.text(),feed.url.text());
        }
      }
    }
public:
  FXArray<FeedLink> links;
public:
  HtmlFeedParser() {
    }
  ~HtmlFeedParser() {
    }
  };

//-----------------------------------------------------------------------------








void unescape_html(FXString & value) {
  value.substitute("&#38;","&");
  }


void parse_duration(const FXString & value,FXuint & d) {
  FXint n = value.contains(":");
  FXint hh=0,mm=0,ss=0;
  switch(n) {
    case 2: value.scan("%d:%d:%d",&hh,&mm,&ss); break;
    case 1: value.scan("%d:%d",&mm,&ss); break;
    case 0: value.scan("%d",&ss); break;
    };
  d=(hh*3600)+(mm*60)+ss;
  }



struct RssItem {
  FXString id;
  FXString url;
  FXString title;
  FXString description;
  FXint    length;
  FXuint   time;
  FXTime   date;

  RssItem() { clear(); }

  void clear() {
    id.clear();
    url.clear();
    title.clear();
    description.clear();
    length=0;
    time=0;
    date=0;
    }
  };

struct RssFeed {
  FXString title;
  FXString description;
  FXString category;
  FXTime   date;

  RssFeed() : date(0) {}

  FXArray<RssItem> items;


  void debug() {
    fxmessage("      title: %s\n",title.text());
    fxmessage("description: %s\n",description.text());
    fxmessage("   category: %s\n",category.text());
    fxmessage("       date: %s\n",FXSystem::universalTime(date).text());
    for (FXint i=0;i<items.no();i++) {
      fxmessage("----\n");
      fxmessage("         url: %s\n",items[i].url.text());
      fxmessage("      length: %d\n",items[i].length);
      fxmessage("       title: %s\n",items[i].title.text());
      fxmessage(" description: %s\n",items[i].description.text());
      fxmessage("          id: %s\n",items[i].id.text());
      fxmessage("    duration: %d\n",items[i].time);
      fxmessage("        date: %s\n",FXSystem::universalTime(items[i].date).text());
      }
    }
  };


class RssParser : public XmlParser {
public:
  RssFeed  feed;
  RssItem  item;
  FXString value;
protected:
  FXint begin(const FXchar *element,const FXchar** attributes){
    switch(node()) {
      case Elem_None:
        if (compare(element,"rss")==0)
          return Elem_RSS;
        break;
      case Elem_RSS:
        if (compare(element,"channel")==0)
          return Elem_Channel;
        break;
      case Elem_Channel:
        if (compare(element,"item")==0)
          return Elem_Item;
        else if (comparecase(element,"title")==0)
          return Elem_Channel_Title;
        else if (comparecase(element,"description")==0)
          return Elem_Channel_Description;
        else if (comparecase(element,"category")==0)
          return Elem_Channel_Category;
        else if (comparecase(element,"itunes:category")==0)
          parse_itunes_category(attributes);
        else if (comparecase(element,"pubdate")==0)
          return Elem_Channel_Date;
        break;
      case Elem_Item:
        if (comparecase(element,"enclosure")==0) {
          if (attributes)
            parse_enclosure(attributes);
          }
        else if (comparecase(element,"title")==0)
          return Elem_Item_Title;
        else if (comparecase(element,"description")==0)
          return Elem_Item_Description;
        else if (comparecase(element,"guid")==0)
          return Elem_Item_Guid;
        else if (comparecase(element,"pubdate")==0)
          return Elem_Item_Date;
        else if (comparecase(element,"itunes:duration")==0)
          return Elem_Item_Duration;
        break;
      default: break;
      }
    return 0;
    }

  void data(const FXchar * data,FXint len){
    switch(node()) {
      case Elem_Item_Title         : item.title.append(data,len); break;
      case Elem_Item_Description   : item.description.append(data,len); break;
      case Elem_Item_Guid          : item.id.append(data,len); break;
      case Elem_Item_Date          : value.append(data,len); break;
      case Elem_Item_Duration      : value.append(data,len); break;
      case Elem_Channel_Title      : feed.title.append(data,len); break;
      case Elem_Channel_Description: feed.description.append(data,len); break;
      case Elem_Channel_Category   : feed.category.append(data,len); break;
      case Elem_Channel_Date       : value.append(data,len); break;
      }
    }
  void end(const FXchar*) {
    //fxmessage("e %d: %s\n",node(),element);
    switch(node()){
      case Elem_Item:
        feed.items.append(item);
        item.clear();
        break;
      case Elem_Item_Date:
        gm_parse_datetime(value,item.date);
        if (feed.date==0) feed.date=item.date;
        value.clear();
        break;
      case Elem_Item_Duration:
        parse_duration(value,item.time);
        value.clear();
        break;
      case Elem_Channel_Date:
        fxmessage("channel date %s\n",value.text());
        gm_parse_datetime(value,feed.date);
        value.clear();
        break;
      }
    }

  void parse_itunes_category(const FXchar** attributes) {
    for (FXint i=0;attributes[i];i+=2) {
      if (comparecase(attributes[i],"text")==0){
        feed.category = attributes[i+1];
        unescape_html(feed.category);
        }
      }
    }

  void parse_enclosure(const FXchar** attributes) {
    for (FXint i=0;attributes[i];i+=2) {
      if (comparecase(attributes[i],"url")==0)
        item.url = attributes[i+1];
      else if (comparecase(attributes[i],"length")==0)
        item.length = FXString(attributes[i+1]).toInt();
      }
    }
public:
  enum {
    Elem_RSS = Elem_Last,
    Elem_Channel,
    Elem_Channel_Title,
    Elem_Channel_Description,
    Elem_Channel_Category,
    Elem_Channel_Date,
    Elem_Item,
    Elem_Item_Title,
    Elem_Item_Description,
    Elem_Item_Guid,
    Elem_Item_Date,
    Elem_Item_Duration,
    };
public:
  RssParser(){}
  ~RssParser(){}
  };

/*--------------------------------------------------------------------------------------------*/



FXString make_podcast_feed_directory(const FXString & title) {
  FXString feed = GMFilename::filter(title,"\'\\#~!\"$&();<>|`^*?[]/.:",GMFilename::NOSPACES);
  FXDir::createDirectories(GMApp::getPodcastDirectory()+PATHSEPSTRING+feed);
  return feed;
  }


/*--------------------------------------------------------------------------------------------*/

class GMImportPodcast : public GMWorker {
FXDECLARE(GMImportPodcast)
protected:
  FXString         url;
  RssParser        rss;
  GMTrackDatabase* db;
  GMQuery          get_feed;
  GMQuery          get_tag;
  GMQuery          add_tag;
  GMQuery          add_feed;
public:
protected:
  GMImportPodcast(){}
public:
  GMImportPodcast(FXApp*app,GMTrackDatabase * database,const FXString & u) : GMWorker(app), url(u), db(database) {
    get_feed = db->compile("SELECT id FROM feeds WHERE url == ?;");
    get_tag  = db->compile("SELECT id FROM tags WHERE name == ?;");
    add_tag  = db->compile("INSERT INTO tags VALUES ( NULL, ? );");
    add_feed = db->compile("INSERT INTO feeds VALUES ( NULL,?,?,?,?,?,?,NULL,NULL);");
    }

  void insert_feed() {
    //printf("%s\n",feed.text());
    FXint feed=0;

    get_feed.set(0,url);
    get_feed.execute(feed);
    if (feed) return;

    fxmessage("adding feed\n");

    FXString feed_dir = make_podcast_feed_directory(rss.feed.title);


    db->begin();
    FXint tag=0;
    get_tag.set(0,rss.feed.category);
    get_tag.execute(tag);
    if (!tag) {
      add_tag.set(0,rss.feed.category);
      tag = add_tag.insert();
      }

    add_feed.set(0,url);
    add_feed.set(1,rss.feed.title);
    add_feed.set(2,rss.feed.description);
    add_feed.set(3,feed_dir);
    add_feed.set(4,tag);
    add_feed.set(5,rss.feed.date);
    FXint feed_id = add_feed.insert();

    GMQuery add_feed_item(db,"INSERT INTO feed_items VALUES ( NULL, ? , ? , ? , NULL, ? , ? , ?, ?, ?, 0)");

   for (FXint i=0;i<rss.feed.items.no();i++) {
      add_feed_item.set(0,feed_id);
      add_feed_item.set(1,rss.feed.items[i].id);
      add_feed_item.set(2,rss.feed.items[i].url);
      add_feed_item.set(3,rss.feed.items[i].title);
      add_feed_item.set(4,rss.feed.items[i].description);
      add_feed_item.set(5,rss.feed.items[i].length);
      add_feed_item.set(6,rss.feed.items[i].time);
      add_feed_item.set(7,rss.feed.items[i].date);
      add_feed_item.execute();
      }
    db->commit();
    }



  long onThreadLeave(FXObject*,FXSelector,void*) {
    FXint code=0;
    if (thread->join(code) && code==0) {
      insert_feed();
      GMPlayerManager::instance()->getTrackView()->refresh();
      }
    else {
      fxmessage("OOPS %d\n",code);
      }


    delete this;
    return 1;
    }

  FXint select_feed(FXArray<FeedLink> links){
    GMThreadDialog dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Select Feed"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,0,0,0,0,0,0,0,0);
    FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
    new GMButton(closebox,fxtr("Subscribe"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,10,5,10,10);
    FXMatrix * matrix = new FXMatrix(main,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS);
    new FXLabel(matrix,fxtr("Feed:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    GMListBox * feedbox = new GMListBox(matrix,NULL,0,LAYOUT_FILL_X|FRAME_LINE);
    for (int i=0;i<links.no();i++){
      feedbox->appendItem(links[i].description);
      }
    feedbox->setNumVisible(FXMIN(feedbox->getNumItems(),9));
    if (dialog.execute(channel)) {
      return feedbox->getCurrentItem();
      }
    return -1;
    }

  FXint run() {
    HttpClient client;

    if (!client.basic("GET",url))
      return 1;

    FXString content = client.getHeader("content-type").before(';');
    fxmessage("%s\n",content.text());
    if (comparecase(content,"application/rss+xml")==0 || comparecase(content,"text/xml")==0) {
      rss.parse(client.body());
      return 0;
      }
    else if (comparecase(content,"text/html")==0) {
      HtmlFeedParser html;
      html.parse(client.body());
      client.close();
      if (html.links.no()) {
        //fxmessage("%s\n",html.links[1].url.text());
        FXint index = select_feed(html.links);
        if (index==-1) return 1;

        FXString uri = html.links[index].url;
        if (uri[0]=='/') {
          uri = FXURL::scheme(url) + "://" +FXURL::host(url) + uri;
          }
        url=uri;

        fxmessage("uri: %s\n",url.text());

        if (client.basic("GET",url)) {
          content = client.getHeader("content-type");
          fxmessage("%s\n",content.text());
          if (comparecase(content,"application/rss+xml")==0 || comparecase(content,"text/xml")==0) {
            rss.parse(client.body());


            // process_feed(db,html.links[index].url,client.body());
            //printf("%s\n",feed.text());
            return 0;
            }
          }
        }
      }
    return 1;
    }
  };

FXDEFMAP(GMImportPodcast) GMImportPodcastMap[]={
  FXMAPFUNC(SEL_COMMAND,GMImportPodcast::ID_THREAD_LEAVE,GMImportPodcast::onThreadLeave),
  };

FXIMPLEMENT(GMImportPodcast,GMWorker,GMImportPodcastMap,ARRAYNUMBER(GMImportPodcastMap));




/*--------------------------------------------------------------------------------------------*/


class GMDownloader : public GMWorker {
FXDECLARE(GMDownloader)
protected:
  FXbool download(const FXString & url,const FXString & filename,FXbool resume=true) {
    HttpClient http;
    FXFile     file;
    FXString   headers;
    FXuint     mode   = FXIO::Writing;
    FXlong     offset = 0;

    if (FXStat::exists(filename) && resume) {
      fxmessage("file exists trying resume");
      mode    = FXIO::ReadWrite|FXIO::Append;
      offset  = FXStat::size(filename);
      if (offset>0) {
        headers = FXString::value("Range: bytes=%ld-\r\nIf-Range: %s\r\n",offset,gm_rfc1123(FXStat::modified(filename)).text());
        fxmessage("%s\n",headers.text());
        }
      }

    if (!file.open(filename,mode)){
      fxmessage("failed to open file\n");
      return false;
      }

    if (!http.basic("GET",url,headers)) {
      fxmessage("failed to get url\n");
      return false;
      }

    if (http.status.code == HTTP_PARTIAL_CONTENT) {
      fxmessage("partial content succes\n");
      FXString range = http.getHeader("content-range");
      fxmessage("%s\n",range.text());
      }
    else if (http.status.code == HTTP_REQUESTED_RANGE_NOT_SATISFIABLE) {
      FXString range = http.getHeader("content-range");
      fxmessage("range: %s\n",range.text());
      //return false;

      fxmessage("partial content failed\n");
      http.discard();
      if (!http.basic("GET",url) || http.status.code!=HTTP_OK)
        return false;
      file.truncate(0);
      }
    else if (http.status.code == HTTP_OK) {
      fxmessage("full content\n");
      file.truncate(0);
      }
    else {
      fxmessage("failed %d\n",http.status.code);
      return false;
      }

    /// Actual transfer
    FXuchar buffer[4096];
    FXlong  n,nbytes=0,ntotal=0,ncontent=http.getContentLength();
    FXTime  timestamp,last = FXThread::time();
    const FXlong seconds   = 1000000000;
    const FXlong msample   = 100000000;
    const FXlong kilobytes = 1024;

    while((n=http.readBody(buffer,4096))>0 && processing) {
      nbytes+=n;
      ntotal+=n;

      /* calculate transfer speed */
      timestamp = FXThread::time();
      if (timestamp-last>msample) {
        FXuint kbps = (nbytes * seconds ) / (kilobytes*(timestamp-last));
        FXuint pct = (FXuint)(((double)ntotal/(double)ncontent) * 100.0);
        nbytes=0;
        last=timestamp;
        }

      /* Write and check out of disk space */
      if (file.writeBlock(buffer,n)<n)
        return false;
      }
    file.close();

    /// Set the modtime
    FXTime modtime=0;
    if (gm_parse_datetime(http.getHeader("last-modified"),modtime) && modtime!=0) {
      FXStat::modified(filename,modtime);
      //gm_set_modification_time(filename,modtime);
      }

    /// Check for partial content
    if (ncontent!=-1 && ncontent<ntotal)
      return false;

    return true;
    }

  GMDownloader(){}
  GMDownloader(FXApp*app) : GMWorker(app) {}
  };

FXIMPLEMENT(GMDownloader,GMWorker,NULL,0);





class GMPodcastDownloader : public GMDownloader {
FXDECLARE(GMPodcastDownloader)
protected:
  FXMutex          mutex;
  FXCondition      condition;


  FXint            id;
  FXString         url;
  FXString         local;
  FXString         localdir;

  GMPodcastSource* src;
  GMTrackDatabase* db;
protected:
  GMPodcastDownloader(){}
public:
  enum {
    ID_DOWNLOAD_COMPLETE = GMWorker::ID_LAST
    };
public:
  GMPodcastDownloader(FXApp*app,GMPodcastSource * s) : GMDownloader(app),src(s), db(s->db) {
    next_task();
    }

  void next_task() {
    GMQuery next_download(db,"SELECT feed_items.id, feed_items.url,feeds.local FROM feed_items,feeds WHERE feeds.id == feed_items.feed AND flags&1 ORDER BY feed_items.date LIMIT 1;");
    id=0;
    url.clear();
    local.clear();
    if (next_download.row()) {
      next_download.get(0,id);
      next_download.get(1,url);
      next_download.get(2,localdir);
      }
    }

  long onDownloadComplete(FXObject*,FXSelector,void*) {
    mutex.lock();
    GMQuery update_feed(db,"UPDATE feed_items SET local = ?, flags = ((flags&~(1<<?))|?) WHERE id == ?;");

    update_feed.set(0,local);
    update_feed.set(1,ITEM_FLAG_QUEUE);
    if (!local.empty())
      update_feed.set(2,ITEM_FLAG_LOCAL);
    else
      update_feed.set(2,ITEM_FLAG_QUEUE);

    update_feed.set(3,id);
    update_feed.execute();

    next_task();
    condition.signal();
    mutex.unlock();
    return 1;
    }

  long onThreadLeave(FXObject*,FXSelector,void*) {
    FXint code=0;
    if (thread->join(code) && code==0) {
      }
    src->downloader = NULL;
    delete this;
    return 1;
    }


  void downloadNext() {
    local = FXPath::name(FXURL::path(url));
    FXDir::createDirectories(GMApp::getPodcastDirectory()+PATHSEPSTRING+localdir);
    if (!download(url,GMApp::getPodcastDirectory()+PATHSEPSTRING+localdir+PATHSEPSTRING+local,true))
      local.clear();
    }

  FXint run() {
    while(id && processing) {
      downloadNext();
      mutex.lock();
      send(FXSEL(SEL_COMMAND,ID_DOWNLOAD_COMPLETE));
      condition.wait(mutex);
      mutex.unlock();
      }
    return 0;
    }
  };

FXDEFMAP(GMPodcastDownloader) GMPodcastDownloaderMap[]={
  FXMAPFUNC(SEL_COMMAND,GMPodcastDownloader::ID_THREAD_LEAVE,GMPodcastDownloader::onThreadLeave),
  FXMAPFUNC(SEL_COMMAND,GMPodcastDownloader::ID_DOWNLOAD_COMPLETE,GMPodcastDownloader::onDownloadComplete),
  };
FXIMPLEMENT(GMPodcastDownloader,GMDownloader,GMPodcastDownloaderMap,ARRAYNUMBER(GMPodcastDownloaderMap));























class GMPodcastUpdater : public GMTask {
protected:
  GMTrackDatabase * db;
protected:
  virtual FXint run();
public:
  GMPodcastUpdater();
  virtual ~GMPodcastUpdater();
  };



GMPodcastUpdater::GMPodcastUpdater() : GMTask(NULL,0) {
  db = GMPlayerManager::instance()->getTrackDatabase();
  }

GMPodcastUpdater::~GMPodcastUpdater() {
  }



FXint GMPodcastUpdater::run() {
  GMQuery all_feeds(db,"SELECT id,url,date FROM feeds;");
  GMQuery all_items(db,"SELECT id,guid FROM feed_items WHERE feed = ?;");
  GMQuery del_items(db,"DELETE FROM feed_items WHERE id == ? AND NOT (flags&2);");
  GMQuery get_item(db,"SELECT id FROM feed_items WHERE feed == ? AND guid == ?;");
  GMQuery set_feed(db,"UPDATE feeds SET date = ? WHERE id = ?;");
  GMQuery add_feed_item(db,"INSERT INTO feed_items VALUES ( NULL, ? , ? , ? , NULL, ? , ? , ?, ?, ?, 0)");


  taskmanager->setStatus("Syncing Podcasts...");
  db->beginTask();

  FXTime date;
  FXString url;
  FXString guid;
  FXint id,item_id;

  while(all_feeds.row()) {
    all_feeds.get(0,id);
    all_feeds.get(1,url);
    all_feeds.get(2,date);

    HttpClient http;

    if (!http.basic("GET",url))
      continue;

    FXString feed = http.body();
    RssParser rss;

    rss.parse(feed);
   // rss.feed.debug();

   fxmessage("%s - %s\n",url.text(),FXSystem::universalTime(date).text());

    FXDict guids;
    for (int i=0;i<rss.feed.items.no();i++)
        guids.insert(rss.feed.items[i].id.text(),(void*)(FXival)1);

    all_items.set(0,id);
    while(all_items.row()){
      all_items.get(0,item_id);
      all_items.get(1,guid);
      if (guids.find(guid.text())==NULL){
        //fxmessage("item removed from feed\n");
        del_items.set(0,item_id);
        del_items.execute();
        }
      else {
        //fxmessage("item still in feed\n");
        }
      }
    all_items.reset();

    for (int i=0;i<rss.feed.items.no();i++){
      item_id=0;
      get_item.set(0,id);
      get_item.set(1,rss.feed.items[i].id);
      get_item.execute(item_id);
      if (item_id==0) {
        //fxmessage("adding %s with date %ld\n",rss.feed.items[i].id.text(),rss.feed.items[i].date);
        add_feed_item.set(0,id);
        add_feed_item.set(1,rss.feed.items[i].id);
        add_feed_item.set(2,rss.feed.items[i].url);
        add_feed_item.set(3,rss.feed.items[i].title);
        add_feed_item.set(4,rss.feed.items[i].description);
        add_feed_item.set(5,rss.feed.items[i].length);
        add_feed_item.set(6,rss.feed.items[i].time);
        add_feed_item.set(7,rss.feed.items[i].date);
        add_feed_item.execute();
        }
      }

    fxmessage("Update date to %s\n",FXSystem::universalTime(rss.feed.date).text());
    set_feed.set(0,rss.feed.date);
    set_feed.set(1,id);
    if (rss.feed.date>date) {
      fxmessage("feed needs updating %s - %s\n",FXSystem::universalTime(rss.feed.date).text(),FXSystem::localTime(rss.feed.date).text());
      }
    else {
      fxmessage("feed is up to date\n");
      }
    }

  db->commitTask();
  return 0;
  }

/*--------------------------------------------------------------------------------------------*/













FXDEFMAP(GMPodcastSource) GMPodcastSourceMap[]={
  FXMAPFUNC(SEL_COMMAND,GMPodcastSource::ID_ADD_FEED,GMPodcastSource::onCmdAddFeed),
  FXMAPFUNC(SEL_COMMAND,GMPodcastSource::ID_REFRESH_FEED,GMPodcastSource::onCmdRefreshFeed),
  FXMAPFUNC(SEL_COMMAND,GMPodcastSource::ID_DOWNLOAD_FEED,GMPodcastSource::onCmdDownloadFeed),
  };
FXIMPLEMENT(GMPodcastSource,GMSource,GMPodcastSourceMap,ARRAYNUMBER(GMPodcastSourceMap));


GMPodcastSource::GMPodcastSource() : db(NULL) {
  }

GMPodcastSource::GMPodcastSource(GMTrackDatabase * database) : db(database),downloader(NULL) {
  FXASSERT(db);
  }

GMPodcastSource::~GMPodcastSource(){
  if (downloader) {
    downloader->stop();
    }
  }


void GMPodcastSource::configure(GMColumnList& list){
  list.no(4);
  list[0]=GMColumn(notr("Date"),HEADER_DATE,GMFeedItem::ascendingDate,GMFeedItem::descendingDate,200,true,true,1);
  list[1]=GMColumn(notr("Title"),HEADER_TITLE,GMStreamTrackItem::ascendingTitle,GMStreamTrackItem::descendingTitle,200,true,true,1);
  list[2]=GMColumn(notr("Status"),HEADER_STATUS,GMFeedItem::ascendingTime,GMFeedItem::descendingTime,200,true,true,1);
  list[3]=GMColumn(notr("Time"),HEADER_TIME,GMFeedItem::ascendingTime,GMFeedItem::descendingTime,200,true,true,1);
  }


FXbool GMPodcastSource::hasCurrentTrack(GMSource * src) const {
  if (src==this) return true;
  return false;
  }

FXbool GMPodcastSource::setTrack(GMTrack & track) const {
  return false;
  }

FXbool GMPodcastSource::getTrack(GMTrack & info) const {
  GMQuery q(db,"SELECT feed_items.url,feed_items.local,feeds.local FROM feed_items,feeds WHERE feeds.id == feed_items.feed AND feed_items.id == ?;");
  FXString local;
  FXString localdir;
  q.set(0,current_track);
  if (q.row()) {
    q.get(0,info.mrl);
    q.get(1,local);
    q.get(2,localdir);
    if (!local.empty()){
      info.mrl = GMApp::getPodcastDirectory() + PATHSEPSTRING + localdir + PATHSEPSTRING + local;
      }
    }
//  q.execute(current_track,info.mrl);
  return true;
  }

FXString GMPodcastSource::getTrackFilename(FXint id) const{
  fxmessage("id %d\n",id);
  return FXString::null;
  }


FXbool GMPodcastSource::source_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("New Feed…\t\t"),NULL,this,ID_ADD_FEED);
  new GMMenuCommand(pane,fxtr("Refresh\t\t"),NULL,this,ID_REFRESH_FEED);
  return false;
  }

FXbool GMPodcastSource::track_context_menu(FXMenuPane * pane){
  new GMMenuCommand(pane,fxtr("Download\t\t"),NULL,this,ID_DOWNLOAD_FEED);
  return true;
  }

FXbool GMPodcastSource::listTags(GMList * list,FXIcon * icon){
  GMQuery q(db,"SELECT id,name FROM tags WHERE id in (SELECT DISTINCT(tag) FROM feeds);");
  FXint id;
  const FXchar * c_title;
  while(q.row()){
      q.get(0,id);
      c_title = q.get(1);
      list->appendItem(c_title,icon);
      }
  return true;
  }


FXbool GMPodcastSource::listAlbums(GMAlbumList *list,const FXIntList &,const FXIntList &){
  GMQuery q(db,"SELECT id,title FROM feeds");
  const FXchar * c_title;
  FXint id;
  GMAlbumListItem* item;
  while(q.row()){
      q.get(0,id);
      c_title = q.get(1);

      item = new GMAlbumListItem(FXString::null,c_title,0,id);
      list->appendItem(item);
      }
  return true;
  }

static void gm_query_make_selection(const FXIntList & list,FXString & selection){
  if (list.no()>1) {
    selection=FXString::value(" IN ( %d",list[0]);
    for (FXint i=1;i<list.no();i++){
      selection+=FXString::value(",%d",list[i]);
      }
    selection+=") ";
    }
  else if(list.no()==1) {
    selection=FXString::value(" == %d ",list[0]);
    }
  }

FXbool GMPodcastSource::listTracks(GMTrackList * tracklist,const FXIntList & albumlist,const FXIntList & /*genre*/){
  FXString selection;
  gm_query_make_selection(albumlist,selection);
  FXString query = "SELECT id,title,time,date,flags FROM feed_items WHERE feed ";
  query+=selection;

  GMQuery q(db,query.text());
  const FXchar * c_title;
  FXint id;
  FXTime date;
  FXuint time;
  FXuint flags;

  while(q.row()){
      q.get(0,id);
      c_title = q.get(1);
      q.get(2,time);
      q.get(3,date);
      q.get(4,flags);
      GMFeedItem* item = new GMFeedItem(id,c_title,date,time,flags);
      tracklist->appendItem((GMTrackItem*)item);
      }
  return true;
  }


long GMPodcastSource::onCmdRefreshFeed(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->runTask(new GMPodcastUpdater);
  return 1;
  }


long GMPodcastSource::onCmdDownloadFeed(FXObject*,FXSelector,void*){
  FXIntList tracks;
  GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
  GMQuery queue_tracks(db,"UPDATE feed_items SET flags = (flags|1) WHERE id == ?;");
  db->begin();
  for (FXint i=0;i<tracks.no();i++){
    queue_tracks.set(0,tracks[i]);
    queue_tracks.execute();
    }
  db->commit();
  if (downloader==NULL) {
    downloader = new GMPodcastDownloader(FXApp::instance(),this);
    downloader->start();
    }
  return 1;
  }

long GMPodcastSource::onCmdAddFeed(FXObject*,FXSelector,void*){
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Subscribe to Podcast"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,0,0,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("Subscribe to Podcast"),fxtr("Specify url and description of new station"),NULL);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("Subscribe"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new FXSeparator(&dialog,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,10,5,10,10);
  FXMatrix * matrix = new FXMatrix(main,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS);
  new FXLabel(matrix,fxtr("Location"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  GMTextField * location_field = new GMTextField(matrix,40,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK);
  if (dialog.execute()) {
    FXString url=location_field->getText().trim();
    if (!url.empty()) {
      GMImportPodcast * podcast = new GMImportPodcast(FXApp::instance(),db,url);
      podcast->start();
      }
    }
  return 1;
  }


