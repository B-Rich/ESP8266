object Form1: TForm1
  Left = 231
  Top = 132
  Width = 322
  Height = 189
  Caption = ' TCP Test Client'
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
  object Label1: TLabel
    Left = 142
    Top = 19
    Width = 38
    Height = 13
    Caption = 'Host IP:'
  end
  object Label2: TLabel
    Left = 146
    Top = 43
    Width = 30
    Height = 13
    Caption = 'My IP:'
  end
  object Label3: TLabel
    Left = 146
    Top = 67
    Width = 22
    Height = 13
    Caption = 'Port:'
  end
  object Edit1: TEdit
    Left = 72
    Top = 96
    Width = 229
    Height = 21
    TabOrder = 0
    Text = 'A Test Message'
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 132
    Width = 306
    Height = 19
    Panels = <>
    SimplePanel = True
    SimpleText = 'Conenction Closed'
  end
  object SendButton1: TButton
    Left = 10
    Top = 94
    Width = 55
    Height = 23
    Caption = 'Send'
    TabOrder = 2
    OnClick = SendButton1Click
  end
  object Button1: TButton
    Left = 10
    Top = 16
    Width = 55
    Height = 25
    Caption = 'Open'
    TabOrder = 3
    OnClick = Button1Click
  end
  object CloseButton1: TButton
    Left = 10
    Top = 47
    Width = 55
    Height = 25
    Caption = 'Close'
    TabOrder = 4
    OnClick = CloseButton1Click
  end
  object HostEdit1: TEdit
    Left = 184
    Top = 16
    Width = 114
    Height = 21
    TabOrder = 5
    Text = '192.168.1.4'
  end
  object MyIPEdit1: TEdit
    Left = 184
    Top = 40
    Width = 114
    Height = 21
    TabOrder = 6
    Text = '192.168.1.4'
  end
  object PortEdit1: TEdit
    Left = 184
    Top = 64
    Width = 114
    Height = 21
    TabOrder = 7
    Text = '1024'
  end
  object ClientSocket1: TClientSocket
    Active = False
    Address = '192.168.1.4'
    ClientType = ctNonBlocking
    Host = '192.168.1.4'
    Port = 1024
    Left = 88
    Top = 16
  end
end
