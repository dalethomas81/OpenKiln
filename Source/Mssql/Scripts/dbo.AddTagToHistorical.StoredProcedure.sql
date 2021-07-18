USE [automation_historical]
GO
/****** Object:  StoredProcedure [dbo].[AddTagToHistorical]    Script Date: 7/18/2021 12:20:45 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- =============================================
-- Author:		Dale Thomas
-- Create date: 2018-06-21
-- Description:	Insert a tag into the historical db
-- =============================================
CREATE PROCEDURE [dbo].[AddTagToHistorical]
	-- Add the parameters for the stored procedure here
	@TAGNAME nVarchar(255) = 0,
	@TYPE nVarchar(255) = 'Analog_Float'
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- insert statements for procedure here
	DECLARE @TagType as smallint
	DECLARE @TagDataType as smallint

	IF @TYPE = 'Analog_Long'
	BEGIN
		SET @TagType = 2
		SET @TagDataType = 0
	END
	IF @TYPE = 'Analog_Float'
	BEGIN
		SET @TagType = 2
		SET @TagDataType = 1
	END
	IF @TYPE = 'Digital_Long'
	BEGIN
		SET @TagType = 3
		SET @TagDataType = 0
	END
	IF @TYPE = 'String_String'
	BEGIN
		SET @TagType = 4
		SET @TagDataType = 2
	END
	
	--get tag index, tagtype, tagdatatype
	INSERT INTO TagTable (TagName, TagType, TagDataType, [Status]) VALUES (@TAGNAME, @TagType, @TagDataType, 'Active')

END
GO
