'use strict';

require('async-tree');
const electron = require('electron')
const {app, BrowserWindow, ipcMain} = electron

var mainWindow = null;

app.on('ready', function() {
    mainWindow = new BrowserWindow({
        frame: false,
        resizable: false,
        height: 600,
        width: 800
    });

    mainWindow.loadURL('file://' + __dirname + '/app/index.html');


});

ipcMain.on('close-main-window', function () {
    app.quit();
});
