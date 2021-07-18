USE [automation_historical]
GO
/****** Object:  Table [dbo].[FloatTable]    Script Date: 7/18/2021 12:20:44 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[FloatTable](
	[RecordNumber] [bigint] NULL,
	[TransactionDateTime] [datetime] NULL,
	[DateAndTime] [datetime] NULL,
	[Millitm] [smallint] NULL,
	[TagIndex] [smallint] NULL,
	[Val] [float] NULL,
	[Status] [nvarchar](1) NULL,
	[Marker] [nvarchar](1) NULL
) ON [PRIMARY]

GO
