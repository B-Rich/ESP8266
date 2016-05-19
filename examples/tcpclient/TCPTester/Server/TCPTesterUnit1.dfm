object Form1: TForm1
  Left = 229
  Top = 128
  Width = 410
  Height = 287
  Caption = ' Simple TCP Server'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 13
  object Memo1: TMemo
    Left = 0
    Top = 28
    Width = 394
    Height = 202
    Align = alClient
    Lines.Strings = (
      'Memo1')
    TabOrder = 0
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 230
    Width = 394
    Height = 19
    Panels = <>
    SimplePanel = True
    SimpleText = 'Conenction Closed'
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 394
    Height = 28
    Align = alTop
    BevelOuter = bvLowered
    TabOrder = 2
    object Label1: TLabel
      Left = 264
      Top = 8
      Width = 22
      Height = 13
      Caption = 'Port:'
    end
    object OpenButton1: TButton
      Left = 2
      Top = 2
      Width = 65
      Height = 24
      Caption = 'Open'
      TabOrder = 0
      OnClick = OpenButton1Click
    end
    object CloseButton1: TButton
      Left = 72
      Top = 2
      Width = 65
      Height = 24
      Caption = 'Close'
      TabOrder = 1
      OnClick = CloseButton1Click
    end
    object PortEdit1: TEdit
      Left = 288
      Top = 4
      Width = 61
      Height = 21
      TabOrder = 2
      Text = '1024'
    end
  end
  object ServerSocket1: TServerSocket
    Active = False
    Port = 1025
    ServerType = stNonBlocking
    OnAccept = ServerSocket1Accept
    OnClientConnect = ServerSocket1ClientConnect
    OnClientDisconnect = ServerSocket1ClientDisconnect
    OnClientRead = ServerSocket1ClientRead
    Left = 144
  end
end
