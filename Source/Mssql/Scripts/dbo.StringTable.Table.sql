USE [automation_historical]
GO
/****** Object:  Table [dbo].[StringTable]    Script Date: 7/18/2021 12:20:45 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[StringTable](
	[RecordNumber] [bigint] NULL,
	[TransactionDateTime] [datetime] NULL,
	[DateAndTime] [datetime] NULL,
	[Millitm] [smallint] NULL,
	[TagIndex] [smallint] NULL,
	[Val] [nvarchar](255) NULL,
	[Status] [nvarchar](1) NULL,
	[Marker] [nvarchar](1) NULL
) ON [PRIMARY]

GO
