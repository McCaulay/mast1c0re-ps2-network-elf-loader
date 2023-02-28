import wx
import os
import re
import json
import socket
import sys
import struct

MAGIC = 0x0000EA6E
PORT = 9045

def getCurrentDirectory():
    if getattr(sys, 'frozen', False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(__file__)

class MainFrame(wx.Frame):
    def __init__(self):
        super().__init__(parent=None, title='mast1c0re File Loader', size=(455, 150))

        self.panel = wx.Panel(self)

        # Select File
        self.file = wx.TextCtrl(self.panel, pos=(5, 5), size=(300, -1))
        self.btnSelectFile = wx.Button(self.panel, label='Select File', pos=(310, 5), size=(130, -1))
        self.btnSelectFile.Bind(wx.EVT_BUTTON, self.selectFile)

        # IP Address
        self.txtIp = wx.TextCtrl(self.panel, pos=(5, 40), size=(435, -1))
        self.txtIp.SetHint('192.168.0.10')

        # Load
        self.btnLoad = wx.Button(self.panel, label='Load', pos=(5, 75), size=(435, -1))
        self.btnLoad.Bind(wx.EVT_BUTTON, self.load)

        # Load config
        try:
            with open(os.path.join(getCurrentDirectory(), 'mast1c0re-file-loader.json')) as f:
                self.config = json.load(f)
                if 'ip' in self.config:
                    self.txtIp.SetValue(self.config['ip'])
                if 'file' in self.config:
                    self.file.SetValue(self.config['file'])
        except:
            self.config = {}

        self.Show()

    def selectFile(self, event):
        directory = os.getcwd()

        if 'file' in self.config:
            directory = os.path.dirname(self.config['file'])

        # Select file dialog
        dialog = wx.FileDialog(self,
            message="Choose a file",
            defaultDir=directory,
            wildcard="PS2 File (*.elf,*.iso)|*.elf;*.iso",
            style=wx.FD_OPEN | wx.FD_CHANGE_DIR
        )

        if dialog.ShowModal() == wx.ID_OK:
            self.file.SetValue(dialog.GetPath())

    def load(self, event):
        # Validate IP address
        ip = self.txtIp.GetValue()
        if not re.match(r"^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$", ip):
            wx.MessageBox('Invalid IP address!', 'Error', wx.OK)
            return

        # Validate filepath
        filepath = self.file.GetValue()
        if not os.path.exists(filepath):
            wx.MessageBox('File does not exist!', 'Error', wx.OK)
            return

        # Get filesize
        stats = os.stat(filepath)

        # Update progress bar
        progress = wx.ProgressDialog('Uploading', 'Sending file to console...', stats.st_size, self.panel)

        with open(filepath, 'rb') as f:
            try:
                # Connect to console
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(3)
                sock.connect((ip, PORT))

                # Send magic
                sock.sendall(struct.pack('<I', MAGIC))

                # Send filesize
                sock.sendall(struct.pack('<Q', stats.st_size))

                # Loop in chunks of 4096
                sent = 0
                while True:
                    data = f.read(4096)
                    if data == b'':
                        break
                    sock.sendall(data)
                    sent += len(data)
                    progress.Update(sent)
                    wx.Yield()

                # Close connection
                sock.close()
            except socket.error:
                wx.MessageBox('Failed to connect!', 'Error', wx.OK)

        progress.Close()

        # Save values to config
        with open(os.path.join(getCurrentDirectory(), 'mast1c0re-file-loader.json'), 'w') as f:
            f.write(json.dumps({
                'ip': ip,
                'file': filepath,
            }, indent=4))

if __name__ == '__main__':
    app = wx.App()
    frame = MainFrame()
    app.MainLoop()