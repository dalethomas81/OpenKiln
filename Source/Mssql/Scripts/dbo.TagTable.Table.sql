USE [automation_historical]
GO
/****** Object:  Table [dbo].[TagTable]    Script Date: 7/18/2021 12:20:45 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[TagTable](
	[TagName] [nvarchar](255) NULL,
	[TagIndex] [smallint] IDENTITY(7,1) NOT NULL,
	[TagType] [smallint] NULL,
	[TagDataType] [smallint] NULL,
	[Status] [varchar](50) NULL,
 CONSTRAINT [PK_TagTable] PRIMARY KEY CLUSTERED 
(
	[TagIndex] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
