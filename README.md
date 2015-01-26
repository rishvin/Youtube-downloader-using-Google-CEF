Youtube downloader is a multi file downloader build on top of Chrome CEF app.
It uses chrome browser framework for all sort of DOM related operations.

Following is the short description of the code :

Youtube downloader code is split into 2 folders :

One residing in the youtubeDownloader folder and second inside the cefclient folder.

The one residign inside youtubeDownloader folder contains code to parse the html code and downloader code 
for downloading text and the binary file(videos). Downloading of the file is done using CURL library.

The one residing in the cefclient folder does communication with the browser. It also include threads to spwan multiple
instances of the downloaders resising in the youtubeDownloader folder. It also includes a kind of garbage collector to free
the resource once the video has been downloaded.
