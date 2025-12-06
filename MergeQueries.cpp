/**
 * 
 * SUB-SYSTEM: Composite Merge Queries
 * 
 * FILENAME: MergeQueries.cpp
 * 
 * DESCRIPTION: SQL query string definitions and SQL builder functions
 *              All SQL is nanodbc-compatible using ? parameter placeholders
 * 
 * NOTES: This file contains ONLY SQL strings and SQL builders
 *        No modernization required - SQL is already database-agnostic
 *        All queries use ? placeholders which work with both ATL OLE DB and nanodbc
 *        SQL builder functions create multi-statement batches for stored procedure calls
 *        
 * COMPATIBILITY: ✅ Legacy ATL OLE DB  ✅ Modern nanodbc
 *
 * AUTHOR: Original SQL maintained; Documentation updated 2025-11-26
 *
 **/

#include "MergeQueries.h"
#include <stdio.h>
#include <string.h>

// Simple Query Strings
const char* SQL_SelectAllMembersOfAComposite = 
    "SELECT id FROM compport cp1 \n"
    "WHERE ownerid = ? \n"
    "AND included='Y' \n"
    "AND DateRangeBegin = (SELECT MAX(DateRangeBegin) FROM compport cp2 \n"
    "        WHERE cp2.ownerid = cp1.ownerid AND \n"
    "              cp2.id = cp1.id  AND cp2.DateRangeBegin <= ? ) ";

const char* SQL_SelectSegmainForPortfolio = 
    "Select id, owner_id, segmenttype_id, segment_name, \n"
    "		segment_abbrev, isinactive, seglevel, calculated, sequence_no \n"
    "from segmain where owner_id = ? \n"
    "order by segmenttype_id";

const char* SQL_SelectTransFor = 
    "SELECT id, trans_no, trans.tran_type, sec_no, wi, \n"
    "	sec_xtend, acct_type, secid, sec_symbol, units, \n"
    "	orig_face, unit_cost, tot_cost, orig_cost, \n"
    "	pcpl_amt, opt_prem, amort_val, basis_adj, \n"
    "	comm_gcr, net_comm, comm_code, sec_fees, \n"
    "	misc_fee1, fee_code1, misc_fee2, fee_code2, \n"
    "	accr_int, income_amt, net_flow, broker_code, \n"
    "	broker_code2, trd_date, stl_date, eff_date, \n"
    "	entry_date, taxlot_no, xref_trans_no, \n"
    "	pend_div_no, rev_trans_no, rev_type, \n"
    "	new_trans_no, orig_trans_no, block_trans_no, \n"
    "	x_id, x_trans_no, x_sec_no, x_wi, \n"
    "	x_sec_xtend, x_acct_type, x_secid, curr_id, \n"
    "	curr_acct_type, inc_curr_id, inc_acct_type, \n"
    "	x_curr_id, x_curr_acct_type, sec_curr_id, \n"
    "	accr_curr_id, base_xrate, inc_base_xrate, \n"
    "	sec_base_xrate, accr_base_xrate, sys_xrate, \n"
    "	inc_sys_xrate, base_open_xrate, sys_open_xrate, \n"
    "	open_trd_date, open_stl_date, open_unit_cost, \n"
    "	orig_yld, eff_mat_date, eff_mat_price, \n"
    "	acct_mthd, trans_srce, adp_tag, div_type, \n"
    "	div_factor, divint_no, roll_date, perf_date, \n"
    "	misc_desc_ind, trans.dr_cr, bal_to_adjust, cap_trans, \n"
    "	safek_ind, dtc_inclusion, dtc_resolve, \n"
    "	recon_flag, recon_srce, income_flag, letter_flag, \n"
    "	ledger_flag, gl_flag, created_by, create_date, \n"
    "	create_time, post_date, bkof_frmt, bkof_seq_no, \n"
    "	dtrans_no, price, restriction_code \n"
    "FROM trans, trantype \n"
    "WHERE trans.tran_type = trantype.tran_type and trans.dr_cr = trantype.dr_cr \n"
    "		and id = ? AND eff_date >= ? AND eff_date <= ? \n"
    "		and rev_trans_no = 0 \n"
    "		and created_by<>'PORTMOV' and perf_impact<>'X' \n"
    "ORDER BY eff_date, sec_no, wi";

const char* SQL_InsertContacts = 
    "SELECT	id, contacttype, uniquename, abbrev, \n"
    "		description, address1, address2, address3, \n"
    "		city_id, state_id, zip, country_id, \n"
    "		phone1, phone2, fax1, fax2, webaddress, domicile_id \n"
    " FROM CONTACTS WHERE uniquename = ? and contacttype = ?";

const char* SQL_DeleteCompMemTransEx = 
    "delete from MAPCOMPMEMTRANSEX  \n"
    "									where CompDate=? and CompID=? ";

const char* SQL_InsertMapCompMemTransEx = 
    "INSERT into MAPCOMPMEMTRANSEX \n"
    "	(CompDate, \n"
    "	CompID, \n"
    "	CompTrans, \n"
    "	CompMem, \n"
    "	CompMemTrans) \n"
    " VALUES (?, ?, ?, ?, ?) ";

const char* SQL_CopySummaryData = 
    "insert into %DEST_TABLE_NAME%  WITH (ROWLOCK) \n"
    "									 select * from %SRC_TABLE_NAME% \n"
    "									 where portfolio_id = ? and perform_date = ?";

const char* SQL_DeleteMergeSessionData = 
    "DELETE FROM Merge_SData WITH (ROWLOCK) WHERE SessionID = ? \n"
    "	DELETE FROM Merge_CompSegMap WITH (ROWLOCK) WHERE SessionID = ?  \n"
    "	DELETE FROM Merge_Compport WITH (ROWLOCK) WHERE SessionID = ?  \n"
    "	DELETE FROM Merge_UV WITH (ROWLOCK) WHERE SessionID = ?  \n"
    "		 ";

const char* SQL_UpdateUnitvalueMonthlyIPV = 
    "UPDATE unitvalue WITH (ROWLOCK) SET ror_source='4' \n"
    "	FROM Merge_CompSegMap \n"
    "	WHERE SessionID = ? AND unitvalue.id = MemberSegID \n"
    "	AND uvdate >= ? AND uvdate < ? \n"
    "	AND ror_source='3' AND DATEPART(dd, DATEADD(dd, 1, uvdate))=1 \n"
    "	AND EXISTS (SELECT * FROM Portmain P WHERE P.ID = MemberPortID AND PeriodType = 'ptMonthly') \n ";

const char* SQL_DeleteMergeUVGracePeriod = 
    "IF EXISTS (SELECT * FROM \n"
    "	  portcategory p, userdefcattypes udc, userdefcatnames udn \n"
    "	  WHERE p.portid = ? AND p.categorytypeid = udc.id \n"
    "	  AND udc.name IN (SELECT value FROM sysvalues \n"
    "	  WHERE name = 'GracePeriodCatType') \n"
    "	  AND p.categorytypeid = udn.categorytypeid \n"
    "	  AND p.categoryid = udn.id \n"
    "	  AND udn.name IN (SELECT value FROM sysvalues \n"
    "	  WHERE name = 'GracePeriodCatName')) \n"
    "   BEGIN \n"
    "	  UPDATE Merge_UV WITH (ROWLOCK) \n"
    "	  SET unitvalue=-999, fudge_factor=1.1 \n"
    "	  WHERE EXISTS ( SELECT * FROM ( \n"
    "		 SELECT id, DATEADD(mm,(SELECT CAST(value AS int) \n"
    "			   FROM sysvalues WHERE name = 'GracePeriodLen'), \n"
    "			   DateFirstIncluded) AS GracePerodEnd \n"
    "		 FROM (SELECT id, MIN(DateRangeBegin) AS DateFirstIncluded \n"
    "			   FROM compport cp WHERE ownerid = ? AND included='Y' \n"
    "			   AND DateRangeBegin > '12/31/1899'\n"
    "			   GROUP BY id ) T \n"
    "		 ) GPE \n"
    "		 WHERE GPE.id = Merge_UV.portfolio_id \n"
    "		 AND Merge_UV.uvdate < GPE.GracePerodEnd AND src = 'E') \n"
    "		 AND Merge_UV.SessionID = ? \n"
    "   END ";

const char* SQL_InsertMergeCompSegMap = 
    "INSERT INTO Merge_CompSegMap ( \n"
    "	SessionID, Owner_ID, ID, MemberPortID, MemberSegID, \n"
    "	SegmentType_ID, ParentRuleID, MemberSegType, LevelNumber, \n"
    "	CatValue, TaxRate, Name \n"
    ") VALUES ( \n"
    "	?, ?, ?, ?, ?, \n"
    "	?, ?, ?, ?, \n"
    "	?, ?, ? \n"
    ") ";

// ============================================================================
// Complex Query Builders
// ============================================================================

#define ANY_SQL_000 "SET NOCOUNT ON \n\0"
#define SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED "SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED \n\0"
#define SQL_SET_TRANS_ISOLATION_LVL_COMMITTED "SET TRANSACTION ISOLATION LEVEL READ COMMITTED \n\0"

// Build_Merge_CompSegMap
#define Build_Merge_CompSegMap_SQL_001 "DECLARE @SessionID uniqueidentifier \n" \
    "	SET @SessionID = newid() \n" \
    "	SELECT CAST(@SessionID as varchar(36)) \n" \
    "	\n" \
    "	EXEC dbo.usp_WtdAverageMerge_Compport @SessionID, ?,?,? \n\0"

#define Build_Merge_CompSegMap_SQL_002 	" EXEC dbo.usp_WtdAverageMerge_CompSegmap @SessionID, ?, ?, ?  \n\0"

#define Build_Merge_CompSegMap_SQL_003 "DECLARE @SessionID uniqueidentifier \n" \
    "	SET @SessionID = ? \n" \
    "	SELECT CAST(@SessionID as varchar(36)) \n" \
    "	\n" \
    "   UPDATE Merge_CompSegMap SET EffStartDate = '12/30/1899', EffEndDate = GetDate() \n" \
    "   WHERE sessionid = @sessionid and EffStartDate IS NULL and EffEndDate IS NULL \n" \
    "	EXEC dbo.usp_WtdAverageMerge_Compport @SessionID, ?,?,? \n\0"

void BuildMergeCompSegMap_SQL(char *sSegMapSQL)
{
    strcpy_s(sSegMapSQL, MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sSegMapSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sSegMapSQL, MAXSQLSIZE, Build_Merge_CompSegMap_SQL_001);
    strcat_s(sSegMapSQL, MAXSQLSIZE, Build_Merge_CompSegMap_SQL_002);
    strcat_s(sSegMapSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

void BuildMergeCompport_SQL(char *sCompportSQL)
{
    strcpy_s(sCompportSQL, MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sCompportSQL, MAXSQLSIZE,SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sCompportSQL, MAXSQLSIZE,Build_Merge_CompSegMap_SQL_003);
    strcat_s(sCompportSQL, MAXSQLSIZE,SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

// BuildMergeUV
#define Build_Merge_UV_SQL_000 "DELETE FROM Merge_UV WITH (ROWLOCK) WHERE SessionID = ? \n\0"
#define Build_Merge_UV_SQL_001 "Exec dbo.usp_WtdAverageMerge_UV ?,?,?,? \n\0"

void BuildMergeUV_SQL(char *sUVSQL)
{
    strcpy_s(sUVSQL, MAXSQLSIZE,ANY_SQL_000);
    strcat_s(sUVSQL, MAXSQLSIZE,SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sUVSQL, MAXSQLSIZE,Build_Merge_UV_SQL_000);
    strcat_s(sUVSQL, MAXSQLSIZE,Build_Merge_UV_SQL_001);
    strcat_s(sUVSQL, MAXSQLSIZE,SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

// GetSummarizedDataForCompositeEx
#define GetSummarizedDataForCompositeEx_SQL_Reset "DELETE FROM Merge_SData WITH (ROWLOCK) WHERE SessionID = ? \n\0"
#define GetSummarizedDataForCompositeEx_SQL_000 "exec dbo.usp_WtdAverageMerge_sData ?, ?, ? \n\0"

#define GetSummarizedDataForCompositeEx_SQL_001 "SELECT smm.SessionID, \n" \
    "		sd.portfolio_id, sd.id, sd.perform_date, sd.perform_date, \n" \
    "		CASE WHEN mkt_val < -1e+308 THEN 0 ELSE mkt_val END, \n" \
    "		Book_value, Accr_inc, Accr_Div, Inc_rclm, Div_rclm, \n" \
    "		Net_Flow, Wtd_flow, Purchases, Sales, Income, Wtd_inc, Fees, Wtd_Fees, \n" \
    "		sd.Exch_rate_base, 	PrincipalPayDown, Maturity, Contributions, Withdrawals, \n" \
    "		Expenses, Receipts, IncomeCash, PrincipalCash, FeesOut, Wtd_FeesOut, Transfers, \n" \
    "		TransferIn, TransferOut, EstAnnIncome,  Perform_Type,NotionalFlow, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_002 " \n" \
    "		CASE sd.Perform_type WHEN 'I' THEN net_flow -  fees ELSE wtd_flow - wtd_fees  END, \n" \
    "		CASE sd.Perform_type WHEN 'I' THEN net_flow ELSE wtd_flow END, \n" \
    "		CASE WHEN sd.portfolio_id =  sd.id THEN 0 \n" \
    "      ELSE CASE sd.Perform_type WHEN 'I' THEN income ELSE wtd_inc END END, \n" \
    "		CASE sd.Perform_type WHEN 'I' THEN ISNULL(Fedetax_Inc,0) \n" \
    "      ELSE ISNULL(Wtd_Fedetax_Inc,0) END, \n" \
    "		CASE sd.Perform_type WHEN 'I' THEN ISNULL(Fedatax_Inc,0) \n" \
    "      ELSE ISNULL(Wtd_Fedatax_Inc,0) END, \n" \
    "		CASE sd.Perform_type WHEN 'I' THEN ISNULL(Fedtax_Rclm,0) \n" \
    "      ELSE ISNULL(Wtd_Fedtax_Rclm,0) END, \n" \
    "		CASE sd.Perform_type WHEN 'I' THEN ISNULL(Fedinctax_Wthld,0) \n" \
    "      ELSE ISNULL(Wtd_Fedinctax_Wthld,0) END, \n" \
    "		ISNULL(Fedetax_Accr_Inc, 0), \n" \
    "		ISNULL(Fedatax_Accr_Inc, 0), \n" \
    "		ISNULL(Fedetax_Accr_Div, 0), \n" \
    "ISNULL(Fedatax_Accr_Div, 0), 'O' as src, Cons_fee, ISNULL(Wtd_cons, 0) Wtd_Cons \n\0"

#define GetSummarizedDataForCompositeEx_SQL_005 "UPDATE Merge_SData WITH (ROWLOCK) SET \n" \
    "	Wtd_Flow = TT.Net_Flow * ReweightFactor, \n" \
    "    GWF = (TT.Net_Flow - TT.Fees) * ReweightFactor, \n" \
    "	NWF = (TT.Net_Flow + TT.feesout) * ReweightFactor, \n" \
    "    CNWF = (TT.Net_Flow - (TT.fees- TT.Cons_Fee) * ReweightFactor, \n" \
    "	Wtd_Inc = TT.Wtd_Inc * ReweightFactor, \n" \
    "	Wtd_Fedetax_Inc = TT.Wtd_Fedetax_Inc * ReweightFactor, \n" \
    "	Wtd_Fedatax_Inc = TT.Wtd_Fedatax_Inc * ReweightFactor, \n" \
    "	Wtd_Fedinctax_Wthld = TT.Wtd_Fedinctax_Wthld * ReweightFactor, \n" \
    "	Wtd_FedTax_Rclm = TT.Wtd_FedTax_Rclm * ReweightFactor, \n" \
    "	Perform_Type = 'W' \0"

#define GetSummarizedDataForCompositeEx_SQL_005a "FROM ( \n" \
    "	SELECT Convert(FLOAT, DATEDIFF(DD, perform_date,?))/Convert(Float, (DATEDIFF(DD, ?, ?)+ExtraDaysCnt)) \n" \
    "		AS ReweightFactor, * FROM ( \n" \
    "		SELECT (SELECT MIN(perform_date) \n" \
    "		FROM Merge_Sdata SD2 WITH (ROWLOCK) \n" \
    "						WHERE sd2.portfolio_id = sd.portfolio_id AND sd2.id = sd.id \n" \
    "               AND sd2.perform_date > sd.perform_date) AS EndSubPeriodDate, \n" \
    "						(SELECT CASE Flow_Weight_Method WHEN 0 THEN 0 WHEN 1 THEN 1 WHEN 2 THEN 0.5 ELSE 0 END \n" \
    "						FROM syssetng) AS ExtraDaysCnt, * \n" \
    "FROM Merge_SData sd WITH (READPAST, UPDLOCK) \0"

#define GetSummarizedDataForCompositeEx_SQL_005b " WHERE SessionID = ? AND (perform_type='T' ) \n" \
    "	AND ((Perform_date <> ?) or (wtd_flow = 0)) AND EXISTS \n" \
    "	(SELECT * FROM summdata csd, Merge_CompSegMap csm WITH (ROWLOCK) \n" \
    "         WHERE  csd.id = csm.id AND csm.membersegid = sd.id AND csd.perform_date = ? \n" \
    "          AND csm.SessionID = sd.SessionID ) \n" \
    "	) T ) TT \n" \
    "	WHERE TT.SessionID = Merge_SData.SessionID AND TT.Portfolio_ID = Merge_SData.Portfolio_ID \n" \
    "	AND TT.ID = Merge_SData.ID AND TT.Perform_Date = Merge_SData.Perform_Date \n\0"

#define GetSummarizedDataForCompositeEx_SQL_005c "UPDATE Merge_SData WITH (ROWLOCK) SET \n" \
    "		mkt_val = 0, Book_value = 0, Accr_inc = 0, Accr_Div = 0, \n" \
    "		Inc_rclm = 0, Div_rclm = 0 \n" \
    "	WHERE SessionID = ? AND perform_date > ? AND EXISTS \n" \
    "		(SELECT sd2.* FROM Merge_SData sd1 \n" \
    "			JOIN Merge_Compsegmap AS mc on mc.Sessionid = sd1.sessionid AND mc.MemberSegID = sd1.id \n" \
    "			JOIN Merge_CompSegmap as mc2 on mc2.sessionid = mc.sessionid and mc2.SegmentType_ID = mc.SegmentType_ID \n" \
    "			JOIN Merge_SData as sd2  on sd2.sessionid = mc2.sessionid and sd2.id = mc2.MemberSegID \n" \
    "			WHERE sd1.SessionID = Merge_SData.SessionID \n" \
    "			AND sd1.portfolio_id = Merge_SData.portfolio_id \n" \
    "			AND sd1.id = Merge_SData.id AND Merge_SData.perform_date < sd2.perform_date) \n\0"

#define GetSummarizedDataForCompositeEx_SQL_005d "UPDATE Merge_SData WITH (ROWLOCK) SET \n" \
    "	Wtd_Flow = TT.GWF * ReweightFactor, \n" \
    "	GWF =TT.GWF*ReweightFactor,NWF =TT.NWF*ReweightFactor, \n" \
    "	CNWF =TT.CNWF*ReweightFactor,Wtd_Inc = TT.Wtd_Inc * ReweightFactor, \n" \
    "	Wtd_Fedetax_Inc = TT.Wtd_Fedetax_Inc * ReweightFactor, \n" \
    "	Wtd_Fedatax_Inc = TT.Wtd_Fedatax_Inc * ReweightFactor, \n" \
    "	Wtd_Fedinctax_Wthld = TT.Wtd_Fedinctax_Wthld * ReweightFactor, \n" \
    "	Wtd_FedTax_Rclm = TT.Wtd_FedTax_Rclm * ReweightFactor, \n" \
    "	Perform_Type = 'W' \n\0"

#define GetSummarizedDataForCompositeEx_SQL_005e "FROM ( \n" \
    "	SELECT convert(float, DATEDIFF(DD, perform_date, ?))/(Convert(Float, DATEDIFF(DD, ?, ?)+ExtraDaysCnt)) \n" \
    "		AS ReweightFactor, * FROM ( \n" \
    "		SELECT (SELECT MIN(perform_date) \n" \
    "						FROM Merge_Sdata SD2 \n" \
    "						WHERE sd2.portfolio_id = sd.portfolio_id AND sd2.id = sd.id \n" \
    "               AND sd2.perform_date > sd.perform_date) AS EndSubPeriodDate, \n" \
    "						(SELECT CASE Flow_Weight_Method WHEN 0 THEN 0 WHEN 1 THEN 1 WHEN 2 THEN 0.5 ELSE 0 END \n" \
    "						FROM syssetng) AS ExtraDaysCnt, * \n\0"

#define GetSummarizedDataForCompositeEx_SQL_005f "FROM Merge_SData sd \n" \
    "	WHERE SessionID = ? AND perform_type='I' AND EXISTS \n" \
    "	(SELECT * FROM summdata csd, Merge_CompSegMap csm \n" \
    "   WHERE  csd.id = csm.id AND csm.membersegid = sd.id AND csd.perform_date = ? \n" \
    "   AND csm.SessionID = sd.SessionID ) \n" \
    "	) T ) TT \n" \
    "	WHERE TT.SessionID = Merge_SData.SessionID AND TT.Portfolio_ID = Merge_SData.Portfolio_ID \n" \
    "	AND TT.ID = Merge_SData.ID AND TT.Perform_Date = Merge_SData.Perform_Date \n\0"

#define GetSummarizedDataForCompositeEx_SQL_006 "SELECT SessionID, \n" \
    "		portfolio_id, sd.id, ActualDate AS perform_date, (ActualDate), \n" \
    "		SUM(Mkt_val), SUM(Book_value), SUM(Accr_inc), SUM(Accr_Div), SUM(Inc_rclm), SUM(Div_rclm), \n" \
    "		SUM(Net_Flow), SUM(Wtd_flow), SUM(Purchases), SUM(Sales),	SUM(Income), SUM(Wtd_inc), \n" \
    "		SUM(Fees), SUM(Wtd_Fees), MAX(Exch_rate_base), SUM(PrincipalPayDown), \n" \
    "		SUM(Maturity), SUM(Contributions), SUM(Withdrawals), SUM(Expenses), SUM(Receipts), \n" \
    "		SUM(IncomeCash), SUM(PrincipalCash), SUM(FeesOut), SUM(Wtd_FeesOut), SUM(Transfers), \n" \
    "		SUM(TransferIn), SUM(TransferOut), SUM(EstAnnIncome),  MAX(Perform_Type), SUM(NotionalFlow), SUM(GWF), SUM(NWF), \n" \
    "		SUM(Wtd_Income), SUM(Wtd_Fedetax_Inc), SUM(Wtd_Fedatax_Inc), \n" \
    "		SUM(Wtd_Fedtax_Rclm), SUM(Wtd_Fedinctax_Wthld), \n" \
    "0, 0,	0, 0, 'S' AS src, sum(CNWF), sum(ISNULL(Wtd_cons, 0)) \n" \
    "	FROM Merge_SData sd \n" \
    "	WHERE SessionID = ?	AND perform_date > ? AND perform_date <= ? \n" \
    "	GROUP BY SessionID, ActualDate, portfolio_id, id \n\0"

#define GetSummarizedDataForCompositeEx_SQL_006a "UPDATE Merge_SDATA WITH (ROWLOCK) \n" \
    "	SET Mkt_val = 0, Accr_inc = 0, Accr_div = 0 \n" \
    "	WHERE SessionID = ? AND Perform_date < ? and Perform_date > ? and (DATEDIFF(DD, ?, ?) > 33)  \n\0"

#define GetSummarizedDataForCompositeEx_SQL_007 "DELETE FROM Merge_SData WITH (ROWLOCK) \n" \
    "	WHERE SessionID = ? AND perform_date > ? AND perform_date <= ? AND src = 'O' \n\0"

#define GetSummarizedDataForCompositeEx_SQL_008 "SELECT sm.id as id, \n" \
    "  ISNULL(uvc.unitvalue,100)*(CROR/100 + 1) AS EUV, \n" \
    "		CASE WHEN uvc.stream_begin_date IS NOT NULL then 4 \n" \
    "				 WHEN ABS(emv) < 0.01 THEN 7 \n" \
    "				 WHEN SBD > ? THEN 2 \n" \
    "          WHEN SBD < pm.inceptiondate THEN 2 ELSE 1 END AS ror_source, \n" \
    "  ISNULL(uvc.stream_begin_date, \n" \
    "    CASE WHEN SBD > ? THEN SBD \n" \
    "         WHEN SBD < pm.inceptiondate THEN pm.inceptiondate \n" \
    "         ELSE ? END) AS SBD, \n" \
    "  TTT.segmenttype_id, TTT.ror_type, perform_date, \n" \
    "  nflow,  Wtd_Flow, Mkt_val, Book_value, Accr_inc, Accr_Div, \n" \
    "  Purchases, Sales, TTT.income, Wtd_inc, \n" \
    "  Fees, Wtd_Fees, PrincipalPayDown, TTT.Maturity, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_009 "Contributions - \n" \
    "	(SELECT ISNULL(SUM(amount),0) FROM bankstat b1 WITH (NOLOCK) \n" \
    "	WHERE TransactionCode_ID IN (24) \n" \
    "	AND b1.id IN (SELECT memberportid FROM Merge_CompSegMap sm2 WITH (NOLOCK)  \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND b1.otheraccountid IN (SELECT memberportid FROM Merge_CompSegMap sm2 WITH (NOLOCK)  \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND tdate > ? AND tdate <= ? \n" \
    "	AND EXISTS (SELECT * FROM bankstat b2  WITH (NOLOCK) \n" \
    "		WHERE b2.otheraccountid = b1.id AND b2.tdate = b1.tdate \n" \
    "		AND b1.otheraccountid = b2.id AND ABS(b1.amount - b2.amount) < 0.01 \n" \
    "		AND b2.TransactionCode_ID in (25) \n" \
    "	)) as Contributions, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_010 "Withdrawals - \n" \
    "	(SELECT ISNULL(SUM(amount),0) FROM bankstat b1 WITH (NOLOCK)  \n" \
    "	WHERE TransactionCode_ID IN (25) \n" \
    "	AND b1.id IN (SELECT memberportid FROM Merge_CompSegMap sm2 WITH (NOLOCK)  \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND b1.otheraccountid IN (SELECT memberportid FROM Merge_CompSegMap sm2 WITH (NOLOCK)  \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND tdate > ? AND tdate <= ? \n" \
    "	AND EXISTS (SELECT * FROM bankstat b2  WITH (NOLOCK) \n" \
    "		WHERE b2.otheraccountid = b1.id AND b2.tdate = b1.tdate \n" \
    "		AND b1.otheraccountid = b2.id AND ABS(b1.amount - b2.amount)<0.01 \n" \
    "		AND b2.TransactionCode_ID IN (24) \n" \
    "	)) as Withdrawals, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_011 "Expenses, Receipts, \n" \
    "  IncomeCash, PrincipalCash, FeesOut, Wtd_FeesOut, \n" \
    "  Transfers, TransferIn, TransferOut, EstAnnIncome,cror, NotionalFlow, Perform_type,\n" \
    "  cons_fee, ISNULL(Wtd_cons, 0) Wtd_Cons FROM ( \n\0"

#define GetSummarizedDataForCompositeEx_SQL_012 "SELECT \n" \
    "	segmenttype_id, ror_type, max(ActualDate) perform_date, MIN(SBD) AS SBD, \n" \
    "  CASE WHEN ABS(SUM(WMV)) < 1e-18 THEN 0 ELSE CASE WHEN (SUM(WMV) >=0 or sum(BMV)=0) THEN SUM(WROR)/SUM(WMV)*100  \n" \
    "  ELSE  ((SUM(WROR)/SUM(WMV)*100) * -1 )  END END AS CROR, \n" \
    "  SUM(ROR)/count(*)*100 AS CRORAvg, SUM(BMV) AS BMV, SUM(EMV) AS EMV, SUM(NF) AS NFLOW, \n" \
    "  SUM(Income) AS income, SUM(Fees) AS Fees, SUM(mkt_val) AS Mkt_val, SUM(Book_value) AS Book_value, \n" \
    "  SUM(Accr_inc) AS Accr_inc, SUM(Accr_Div) AS Accr_Div, SUM(Purchases) AS Purchases, SUM(Sales) AS Sales, \n" \
    "  SUM(Wtd_inc) AS Wtd_inc, SUM(Wtd_Fees) AS Wtd_Fees,  SUM(PrincipalPayDown) AS PrincipalPayDown, \n" \
    "	SUM(Maturity) AS Maturity, SUM(Contributions) AS Contributions, SUM(Withdrawals) AS Withdrawals, \n" \
    "  SUM(Expenses) AS Expenses, SUM(Receipts) AS Receipts, SUM(IncomeCash) AS IncomeCash, \n" \
    "	SUM(PrincipalCash) AS PrincipalCash, SUM(FeesOut) AS FeesOut, SUM(Wtd_FeesOut) AS Wtd_FeesOut, SUM(Transfers) AS Transfers, \n" \
    "  SUM(TransferIn) AS TransferIn, SUM(TransferOut) AS TransferOut,SUM(NotionalFlow) as NotionalFlow, \n" \
    "  SUM(EstAnnIncome) AS EstAnnIncome, SUM(WMV) AS wmv, SUM(Wtd_flow) AS Wtd_Flow, max(Perform_type) Perform_type, \n" \
    "  SUM(cons_fee) Cons_fee, Sum(ISNULL(Wtd_Cons,0)) Wtd_Cons \n\0"

#define GetSummarizedDataForCompositeEx_SQL_013 "FROM ( \n" \
    "	SELECT   \n" \
    "	CASE WHEN WeightFactor >=0.0 THEN WeightFactor * (EUV/BUV-1.0) ELSE \n" \
    "	CASE WHEN (EUV/BUV -1) = -1.0 OR ((EMV -NF- (EUV/BUV - 1.0) * wtd_flow)/((EUV/BUV-1.0)+1)) *-1 = Wtd_flow  \n" \
    "	THEN WeightFactor * (euv/buv-1) * -1.0 ELSE \n" \
    "	CASE WHEN bmv=0.0 AND ROUND((EMV-((EMV-NF-(EUV/BUV - 1.0) * wtd_flow)/ ((EUV/BUV-1.0)+ 1))- NF) / (((EMV - NF-(EUV/BUV - 1.0) * wtd_flow) / \n" \
    "	((EUV/BUV - 1.0)+1))+ wtd_flow),1)=ROUND((euv/buv-1.0),1) \n" \
    "	AND ((EMV -NF - (EUV/BUV-1.0) * wtd_flow)/((EUV/BUV-1.0)+1))>0.0 THEN \n" \
    "	WeightFactor * (euv/buv - 1.0)  ELSE WeightFactor * (EUV/BUV - 1.0) *  -1.0  END END END AS WROR, \n" \
    "	WeightFactor AS WMV,(EUV/BUV - 1) AS ROR, *  \n\0"

#define GetSummarizedDataForCompositeEx_SQL_013a "FROM (SELECT \n" \
    "	CASE ror_type/100 \n" \
    "	WHEN 3 THEN \n" \
    "		CASE WHEN EUV = -999 AND FudgeFactor <> 0 THEN 0 ELSE 1 END \n" \
    "	WHEN 2 THEN  \n" \
    "		CASE WHEN EUV = -999 AND FudgeFactor <> 0 THEN 0 ELSE BMV END \n" \
    "	ELSE \n" \
    "		CASE WHEN EUV = -999 AND FudgeFactor <> 0 THEN 0  \n" \
    "		ELSE CASE WHEN ABS(BMV+WF) < 0.01 AND ABS(EUV-BUV)>0.0001*BUV AND BMV <> 0 \n" \
    "      			THEN CASE WHEN Perform_Type='W' THEN 0 \n" \
    "		          WHEN NF<>0 THEN NF ELSE CASE WHEN (ABS(EUV-BUV) > 0) then (ABS(EMV  / ((EUV/BUV - 1)*100))/2 ) ELSE 0 END END  \n" \
    "			     ELSE BMV+WF END END \n" \
    "	END AS WeightFactor, *  \n" \
    "	FROM ( SELECT \n" \
    "		sm.segmenttype_id, sd.ActualDate, sd.Perform_date, rt.id ror_type, \n" \
    "		ISNULL((SELECT CASE WHEN mkt_val < -1e+308 THEN NULL ELSE \n" \
    "			CASE WHEN rt.id % 100  IN (1,3) THEN mkt_val+ accr_inc + accr_div \n" \
    "            WHEN rt.id % 100  IN (2,4,5) THEN mkt_val \n" \
    "            WHEN rt.id % 100  IN (8,9) \n" \
    "                THEN mkt_val+ accr_inc + (Fedetax_Accr_Inc / sm.taxrate - Fedetax_Accr_Inc) \n" \
    "                     + accr_div + (Fedetax_Accr_Div / sm.taxrate - Fedetax_Accr_Div) \n" \
    "            WHEN rt.id % 100  IN (10,11) \n" \
    "                THEN mkt_val+ accr_inc + (Fedatax_Accr_Inc * sm.taxrate - Fedatax_Accr_Inc) \n" \
    "                     + accr_div + (Fedatax_Accr_Div * sm.taxrate - Fedatax_Accr_Div) \n" \
    "            ELSE mkt_val+ accr_inc + accr_div END END \n" \
    "       FROM Merge_SData m1 WITH (NOLOCK)  \n" \
    "       WHERE m1.id = sd.id AND m1.portfolio_id = sd.portfolio_id  \n" \
    "	  AND m1.SegmentTypeID = sd.SegmentTypeID \n" \
    "       AND m1.perform_date = (SELECT ISNULL(MIN(sdm.Perform_date),?) FROM Merge_SData sdm  WITH (NOLOCK)   \n" \
    "						WHERE sessionid = m1.SessionID and m1.SegmentTypeID = sdm.SegmentTypeID \n" \
    "						AND sdm.Perform_Date >= ?) AND m1.SessionID = ? \n" \
    "     ),0) AS BMV, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_013b " ISNULL(CASE WHEN mkt_val < -1e+308 THEN NULL ELSE \n" \
    "	mkt_val+ accr_inc + accr_div END, 0)  AS EMV, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_014 "net_flow AS NF,  wtd_flow, \n" \
    "CASE rt.id % 100  \n" \
    "		WHEN 1 THEN GWF \n" \
    "     WHEN 2 THEN GWF + Wtd_Inc \n" \
    "     WHEN 3 THEN NWF \n" \
    "     WHEN 4 THEN NWF + Wtd_Inc \n" \
    "     WHEN 5 THEN 0 \n" \
    "     WHEN 8 THEN GWF\n" \
    "     WHEN 9 THEN NWF\n" \
    "     WHEN 10 THEN GWF\n" \
    "     WHEN 11 THEN NWF\n" \
    "     WHEN 12 THEN CNWF\n" \
    "     ELSE GWF END AS WF, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_015 "Mkt_val, \n" \
    "	Book_value, Accr_inc, Accr_Div, \n" \
    "	Purchases, Sales, Income, Wtd_inc, \n" \
    "	Fees, Wtd_Fees, PrincipalPayDown, Maturity, \n" \
    "	Contributions, Withdrawals, Expenses, Receipts, \n" \
    "	IncomeCash, PrincipalCash, FeesOut, Wtd_FeesOut, NotionalFlow, \n" \
    "	Transfers, TransferIn, TransferOut, EstAnnIncome, Perform_Type, \n" \
    "	ISNULL(NULLIF((SELECT TOP 1 u1.unitvalue  \n" \
    "		FROM Merge_UV u1 WITH (NOLOCK)  \n" \
    "     WHERE u1.SessionID = uv.SessionID  \n" \
    "     AND u1.id = uv.id AND u1.portfolio_id = uv.portfolio_id \n" \
    "	AND u1.ror_type = uv.ror_type \n" \
    "     AND u1.stream_begin_date = uv.stream_begin_date \n" \
    "     AND uvdate >= ? \n" \
    "     AND uvdate < ? \n" \
    "     AND u1.SegmentTypeID = uv.SegmentTypeID \n" \
    "     AND ror_source in (1,2,3,4,6,7) \n" \
    "     ORDER BY uvdate),0),100) AS BUV, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_015a " ISNULL(uv.stream_begin_date, sd.ActualDate) AS SBD ,\n" \
    "	ISNULL(NULLIF((SELECT TOP 1 u1.unitvalue  \n" \
    "		FROM Merge_UV u1 WITH (NOLOCK)  \n" \
    "     WHERE u1.SessionID = uv.SessionID  \n" \
    "     AND u1.id = uv.id AND u1.portfolio_id = uv.portfolio_id \n" \
    "	AND u1.ror_type = uv.ror_type \n" \
    "     AND u1.stream_begin_date = uv.stream_begin_date \n" \
    "     AND u1.SegmentTypeID = uv.SegmentTypeID \n" \
    "     and u1.uvdate >= ? --datefrom \n" \
    "     AND ror_source in (1,2,3,4,6,7) \n" \
    "     ORDER BY uvdate desc),0),-999) AS badUV, \n" \
    "     unitvalue as EUV,\n" \
    " Fudge_Factor AS FudgeFactor,Cons_fee, ISNULL(Wtd_cons,0) Wtd_Cons \n\0"

#define GetSummarizedDataForCompositeEx_SQL_016 "FROM Merge_CompSegMap sm  WITH (NOLOCK)\n" \
    "	JOIN (SELECT id FROM rtntype WITH (NOLOCK) WHERE lookuptype = 5 AND id NOT IN (6,7) \n" \
    "			UNION SELECT 0 AS id) rt ON 1=1 \n" \
    "	JOIN Merge_SData sd WITH (NOLOCK)  \n" \
    "		ON sd.id = sm.membersegid \n" \
    "		AND sd.SegmentTypeid = sm.SegmentType_ID \n" \
    "		AND perform_date > ? AND perform_date <= ? \n" \
    "		AND Perform_date = (select max(Perform_date) from Merge_sdata WITH (NOLOCK)  \n" \
    "							where Sessionid = sd.Sessionid \n" \
    "							AND segmenttypeid = sd.segmenttypeid) \n" \
    "		AND sd.SessionID = sm.SessionID \n" \
    "	LEFT OUTER JOIN Merge_UV WITH (NOLOCK)  \n" \
    "	ON uv.SessionID = sm.SessionID \n" \
    "	AND uv.id = sd.id \n" \
    "     AND uv.SegmenttypeID = sd.SegmentTypeid \n" \
    "	AND uv.uvdate = sd.perform_date AND rt.id = uv.ror_type \n" \
    "	WHERE sm.SessionID = ?  \n" \
    "     AND (rt.id = uv.ror_type OR rt.id=0) \n" \
    "	) S ) T ) TT \n" \
    "	GROUP BY segmenttype_id, ror_type, actualdate \n" \
    "	) TTT \n" \
    "	JOIN portmain pm WITH (NOLOCK)  ON pm.id = ? AND deletedate IS NULL AND portfoliotype IN (1,6,7) \n\0"

#define GetSummarizedDataForCompositeEx_SQL_017 "LEFT OUTER JOIN \n" \
    "	(SELECT DISTINCT id, segmenttype_id, owner_id, SessionID \n" \
    "	FROM Merge_CompSegMap  WITH (NOLOCK) WHERE SessionID = ?) sm \n" \
    "         ON owner_id = pm.id AND sm.segmenttype_id = TTT.segmenttype_id \n" \
    "	LEFT OUTER JOIN Merge_UV uvc WITH (NOLOCK)  ON uvc.SessionID = sm.SessionID \n" \
    "	AND uvc.id = sm.id \n" \
    "     AND uvc.ror_type = TTT.ror_type AND uvdate >= ? \n" \
    "     AND uvdate < TTT.Perform_Date AND ror_source IN (1,2,3,4,6,7) \n" \
    "	WHERE perform_date IS NOT NULL AND (croravg IS NOT NULL OR TTT.ror_type = 0) \n\0"

#define GetSummarizedDataForCompositeEx_SQL_018 "UNION ALL SELECT sm.id as id,  \n" \
    "  100.0 AS EUV,  4 as ror_source, Perform_date as SBD,   TTT.segmenttype_id,\n" \
    "    TTT.ror_type, perform_date, nflow,  Wtd_Flow, Mkt_val, Book_value, \n" \
    "  Accr_inc, Accr_Div, Purchases, Sales, TTT.income, Wtd_inc, Fees, Wtd_Fees, PrincipalPayDown, \n" \
    "  TTT.Maturity, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_019 "Contributions - \n" \
    "	(SELECT ISNULL(SUM(amount),0) FROM bankstat b1 WITH (NOLOCK)  \n" \
    "	WHERE TransactionCode_ID IN (24) \n" \
    "	AND b1.id IN (SELECT memberportid FROM Merge_CompSegMap sm2 WITH (NOLOCK)  \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND b1.otheraccountid IN (SELECT memberportid FROM Merge_CompSegMap sm2 WITH (NOLOCK)  \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND tdate > ? AND tdate <= ? \n" \
    "	AND EXISTS (SELECT * FROM bankstat b2 WITH (NOLOCK)  \n" \
    "		WHERE b2.otheraccountid = b1.id AND b2.tdate = b1.tdate \n" \
    "		AND b1.otheraccountid = b2.id AND ABS(b1.amount - b2.amount) < 0.01 \n" \
    "		AND b2.TransactionCode_ID in (25) \n" \
    "	)) as Contributions, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_020 "Withdrawals - \n" \
    "	(SELECT ISNULL(SUM(amount),0) FROM bankstat b1  WITH (NOLOCK) \n" \
    "	WHERE TransactionCode_ID IN (25) \n" \
    "	AND b1.id IN (SELECT memberportid FROM Merge_CompSegMap sm2  WITH (NOLOCK) \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND b1.otheraccountid IN (SELECT memberportid FROM Merge_CompSegMap sm2 WITH (NOLOCK) \n" \
    "                WHERE sm2.segmenttype_id = sm.segmenttype_id AND sm2.SessionID = sm.SessionID) \n" \
    "	AND tdate > ? AND tdate <= ? \n" \
    "	AND EXISTS (SELECT * FROM bankstat b2  WITH (NOLOCK) \n" \
    "		WHERE b2.otheraccountid = b1.id AND b2.tdate = b1.tdate \n" \
    "		AND b1.otheraccountid = b2.id AND ABS(b1.amount - b2.amount)<0.01 \n" \
    "		AND b2.TransactionCode_ID IN (24) \n" \
    "	)) as Withdrawals, \n\0"

#define GetSummarizedDataForCompositeEx_SQL_021 "Expenses, Receipts, \n" \
    "  IncomeCash, PrincipalCash, FeesOut, Wtd_FeesOut, \n" \
    "Transfers, TransferIn, TransferOut, EstAnnIncome, 0.0 cror, NotionalFlow, perform_type, cons_fee, ISNULL(Wtd_Cons,0) FROM ( \n\0"

#define GetSummarizedDataForCompositeEx_SQL_022 "SELECT Owner_ID, \n" \
    "	segmenttype_id, ror_type, perform_date, SUM(Net_Flow) AS NFLOW, \n" \
    "	SUM(Income) AS income, SUM(Fees) AS Fees, SUM(mkt_val) AS Mkt_val, SUM(Book_value) AS Book_value, \n" \
    "	SUM(Accr_inc) AS Accr_inc, SUM(Accr_Div) AS Accr_Div, SUM(Purchases) AS Purchases, SUM(Sales) AS Sales, \n" \
    "	SUM(Wtd_inc) AS Wtd_inc, SUM(Wtd_Fees) AS Wtd_Fees,  SUM(PrincipalPayDown) AS PrincipalPayDown, \n" \
    "	SUM(Maturity) AS Maturity, SUM(Contributions) AS Contributions, SUM(Withdrawals) AS Withdrawals, \n" \
    "	SUM(Expenses) AS Expenses, SUM(Receipts) AS Receipts, SUM(IncomeCash) AS IncomeCash, \n" \
    "	SUM(PrincipalCash) AS PrincipalCash, SUM(FeesOut) AS FeesOut, SUM(Wtd_FeesOut) AS Wtd_FeesOut, SUM(Transfers) AS Transfers, \n" \
    "	SUM(TransferIn) AS TransferIn, SUM(TransferOut) AS TransferOut,SUM(NotionalFlow) as NotionalFlow, \n" \
    "	SUM(EstAnnIncome) AS EstAnnIncome, 0.0 AS wmv, SUM(Wtd_flow) AS Wtd_Flow, max(Perform_type) perform_type, \n" \
    "    SUM(cons_fee) AS Cons_Fee, SUM(ISNULL(Wtd_Cons,0)) AS Wtd_Cons	\n\0"

#define GetSummarizedDataForCompositeEx_SQL_025 " FROM Merge_CompSegMap sm WITH (NOLOCK)  \n" \
    "	JOIN (SELECT 0 AS ror_type) rt ON 1=1 \n" \
    "	JOIN Merge_SData sd WITH (NOLOCK)  \n" \
    "		ON sd.id = sm.membersegid\n" \
    "		AND sd.SegmentTypeid = sm.SegmentType_ID \n" \
    "		AND sd.Perform_date = (SELECT MIN(Perform_date) from Merge_sdata WITH (NOLOCK)  \n" \
    "							where Sessionid = sd.Sessionid \n" \
    "							AND segmenttypeid = sd.segmenttypeid) \n" \
    "		AND EXISTS (SELECT S.* FROM Merge_SData S WITH (NOLOCK)  where S.Sessionid = sd.Sessionid \n" \
    "		AND S.segmenttypeid = sd.SegmentTypeID AND S.Perform_Date > sd.Perform_date) \n" \
    "		AND sd.SessionID = sm.SessionID \n" \
    "		AND SD.Perform_Date > ? AND SD.Perform_date < ?  \n" \
    "		WHERE sm.SessionID = ? \n" \
    "	GROUP BY owner_id , segmenttype_id, ror_type, perform_date \n" \
    "	) TTT \n" \
    "	JOIN portmain pm  WITH (NOLOCK) ON pm.id = TTT.Owner_ID AND deletedate IS NULL AND portfoliotype IN (1,6,7) \n\0"

#define GetSummarizedDataForCompositeEx_SQL_027 "LEFT OUTER JOIN \n" \
    "	(SELECT DISTINCT id, segmenttype_id, owner_id, SessionID \n" \
    "	FROM Merge_CompSegMap  WITH (NOLOCK) WHERE SessionID = ?) sm \n" \
    "         ON sm.owner_id = pm.id AND sm.segmenttype_id = TTT.segmenttype_id \n" \
    "		WHERE perform_date IS NOT NULL and TTT.ror_type = 0 \n" \
    "ORDER BY TTT.segmenttype_id, TTT.ror_type desc, perform_date  desc \n\0"

void BuildSQLForCompositeEx(char* sAdjSQL)
{
    strcpy_s(sAdjSQL, MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sAdjSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);

    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_008);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_009);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_010);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_011);

    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_012);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_013);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_013a);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_013b);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_014);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_015);

    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_015a);

    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_016);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_017);

    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_018);

    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_019);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_020);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_021);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_022);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_025);
    strcat_s(sAdjSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_027);

    strcat_s(sAdjSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

void BuildMergeSData_SQL(char *sSQL)
{
    strcpy_s(sSQL, MAXSQLSIZE,ANY_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE,SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sSQL, MAXSQLSIZE,GetSummarizedDataForCompositeEx_SQL_Reset);
    strcat_s(sSQL, MAXSQLSIZE,GetSummarizedDataForCompositeEx_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE,SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

void BuildUpdateMergeSData_SQL(char *sSQL)
{
    strcpy_s(sSQL,MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);

    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_005);
    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_005a);
    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_005b);
    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_005c);

    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_005d);
    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_005e);
    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_005f);

    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_006);

    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_006a);
    strcat_s(sSQL, MAXSQLSIZE, GetSummarizedDataForCompositeEx_SQL_007);

    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

// SummarizeTaxPerf
#define SummarizeTaxPerf_SQL_001 "INSERT INTO TAXPERF WITH (ROWLOCK) \n" \
    "		SELECT smm.owner_id, TTT.* FROM ( \n" \
    "			 SELECT MAX(sm.id) AS id, \n" \
    "			 ? as Perform_date,  \n" \
    "			 SUM(ISNULL(Fedinctax_Wthld,0)) AS Fedinctax_Wthld, \n" \
    "			 SUM(ISNULL(Cum_Fedinctax_Wthld,0)) AS Cum_Fedinctax_Wthld, \n" \
    "        SUM(ISNULL(Wtd_Fedinctax_Wthld,0)) AS Wtd_Fedinctax_Wthld, \n" \
    "        SUM(ISNULL(Fedtax_Rclm,0)) AS Fedtax_Rclm, \n" \
    "        SUM(ISNULL(Cum_Fedtax_Rclm,0)) AS Cum_Fedtax_Rclm, \n" \
    "        SUM(ISNULL(Wtd_Fedtax_Rclm,0)) AS Wtd_Fedtax_Rclm, \n" \
    "        SUM(ISNULL(Fedetax_Inc,0)) AS Fedetax_Inc, \n" \
    "        SUM(ISNULL(Cum_Fedetax_Inc,0)) AS Cum_Fedetax_Inc, \n" \
    "        SUM(ISNULL(Wtd_Fedetax_Inc,0)) AS Wtd_Fedetax_Inc, \n" \
    "        SUM(ISNULL(Fedatax_Inc,0)) AS Fedatax_Inc, \n" \
    "        SUM(ISNULL(Cum_Fedatax_Inc,0)) AS Cum_Fedatax_Inc, \n" \
    "        SUM(ISNULL(Wtd_Fedatax_Inc,0)) AS Wtd_Fedatax_Inc, \n\0"

#define SummarizeTaxPerf_SQL_002 "SUM(ISNULL(Fedetax_Strgl,0)) AS Fedetax_Strgl, \n" \
    "        SUM(ISNULL(Cum_Fedetax_Strgl,0)) AS Cum_Fedetax_Strgl, \n" \
    "        SUM(ISNULL(Wtd_Fedetax_Strgl,0)) AS Wtd_Fedetax_Strgl, \n" \
    "        SUM(ISNULL(Fedetax_Ltrgl,0)) AS Fedetax_Ltrgl, \n" \
    "        SUM(ISNULL(Cum_Fedetax_Ltrgl,0)) AS Cum_Fedetax_Ltrgl, \n" \
    "        SUM(ISNULL(Wtd_Fedetax_Ltrgl,0)) AS Wtd_Fedetax_Ltrgl, \n" \
    "        SUM(ISNULL(Fedetax_Crrgl,0)) AS Fedetax_Crrgl, \n" \
    "        SUM(ISNULL(Cum_Fedetax_Crrgl,0)) AS Cum_Fedetax_Crrgl, \n" \
    "        SUM(ISNULL(Wtd_Fedetax_Crrgl,0)) AS Wtd_Fedetax_Crrgl, \n" \
    "        SUM(ISNULL(Fedatax_Strgl,0)) AS Fedatax_Strgl, \n" \
    "        SUM(ISNULL(Cum_Fedatax_Strgl,0)) AS Cum_Fedatax_Strgl, \n" \
    "        SUM(ISNULL(Wtd_Fedatax_Strgl,0)) AS Wtd_Fedatax_Strgl, \n" \
    "        SUM(ISNULL(Fedatax_Ltrgl,0)) AS Fedatax_Ltrgl, \n" \
    "        SUM(ISNULL(Cum_Fedatax_Ltrgl,0)) AS Cum_Fedatax_Ltrgl, \n" \
    "        SUM(ISNULL(Wtd_Fedatax_Ltrgl,0)) AS Wtd_Fedatax_Ltrgl, \n" \
    "        SUM(ISNULL(Fedatax_Crrgl,0)) AS Fedatax_Crrgl, \n" \
    "        SUM(ISNULL(Cum_Fedatax_Crrgl,0)) AS Cum_Fedatax_Crrgl, \n" \
    "        SUM(ISNULL(Wtd_Fedatax_Crrgl,0)) AS Wtd_Fedatax_Crrgl, \n\0"

#define SummarizeTaxPerf_SQL_003 "SUM(ISNULL(Fedatax_Accr_Inc,0)) AS Fedatax_Accr_Inc, \n" \
    "        SUM(ISNULL(Fedatax_Accr_Div,0)) AS Fedatax_Accr_Div, \n" \
    "        SUM(ISNULL(Fedetax_Accr_Inc,0)) AS Fedetax_Accr_Inc, \n" \
    "        SUM(ISNULL(Fedetax_Accr_Div,0)) AS Fedetax_Accr_Div, \n" \
    "        GETDATE() AS Create_Date, GETDATE() AS Change_Date \n" \
    "			 FROM Merge_SData sd WITH (NOLOCK)  \n" \
    "			 JOIN Merge_CompSegMap sm WITH (NOLOCK)  ON sm.membersegid = sd.id \n" \
    "			 AND sm.SegmentType_ID = sd.SegmentTypeID \n" \
    "			 AND sm.SessionID = sd.SessionID \n" \
    "			 WHERE sd.SessionID = ? \n" \
    "			 AND perform_date > ? AND perform_date <= ? \n" \
    "			 GROUP BY sm.id \n" \
    "			) TTT \n" \
    "			JOIN Merge_CompSegMap smm WITH (NOLOCK)  ON smm.id = TTT.id \n" \
    "			AND smm.SessionID = ? \n\0"

void BuildSummarizeTaxPerf_SQL(char *sSQL)
{
    strcpy_s(sSQL, MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sSQL, MAXSQLSIZE, SummarizeTaxPerf_SQL_001);
    strcat_s(sSQL, MAXSQLSIZE, SummarizeTaxPerf_SQL_002);
    strcat_s(sSQL, MAXSQLSIZE, SummarizeTaxPerf_SQL_003);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

// SummarizeMonthsum
#define SummarizeMonthsum_SQL_001 "INSERT INTO MONTHSUM WITH (ROWLOCK) \n" \
    "	SELECT smm.owner_id, TTT.* FROM ( \n" \
    "		SELECT MAX(sm.id) AS id, \n" \
    "			 ? AS Perform_date, \n" \
    "			CASE WHEN MAX(perform_date) = ? THEN 	\n" \
    "				SUM(CASE WHEN mkt_val < -1e+308 THEN 0 ELSE ISNULL(mkt_val,0) END) \n" \
    "			ELSE -1.7e+308 END AS Mkt_val, \n" \
    "        SUM(ISNULL(Book_value, 0)) AS Book_value, \n" \
    "        SUM(ISNULL(Accr_inc, 0)) AS Accr_inc, SUM(ISNULL(Accr_Div, 0)) AS Accr_Div, \n" \
    "        SUM(ISNULL(Inc_rclm, 0)) AS Inc_rclm, SUM(ISNULL(Div_rclm, 0)) AS Div_rclm, \n" \
    "        SUM(ISNULL(Net_Flow, 0)) AS Net_Flow, 0 AS Cum_Flow, \n" \
    "			 SUM(ISNULL(Wtd_flow, 0)) AS Wtd_flow, \n" \
    "        SUM(ISNULL(Purchases, 0)) AS Purchases, SUM(ISNULL(Sales, 0)) AS Sales, \n" \
    "        SUM(ISNULL(Income, 0)) AS Income, 0 AS Cum_income, \n" \
    "			 SUM(ISNULL(Wtd_inc, 0)) AS Wtd_inc, \n" \
    "        SUM(ISNULL(Fees, 0)) AS Fees, 0 AS Cum_fees, \n" \
    "			 SUM(ISNULL(Wtd_Fees, 0)) AS Wtd_Fees,  \n" \
    "        1 AS Exch_rate_base, 'MV' AS Interval_type, 0 AS Days_since_nond, 0 AS Days_since_Last, \n" \
    "        GETDATE() AS Create_Date, GETDATE() AS Change_Date, 'M' AS Perform_type,  \n" \
    "        SUM(ISNULL(PrincipalPayDown, 0)) AS PrincipalPayDown, SUM(ISNULL(Maturity, 0)) AS Maturity, \n\0"

#define SummarizeMonthsum_SQL_002 "SUM(ISNULL(Contributions, 0)) AS Contributions, SUM(ISNULL(Withdrawals, 0)) AS Withdrawals, \n" \
    "        SUM(ISNULL(Expenses, 0)) AS Expenses, SUM(Receipts, 0) AS Receipts, \n" \
    "        SUM(ISNULL(IncomeCash, 0)) AS IncomeCash, SUM(ISNULL(PrincipalCash, 0)) AS PrincipalCash, \n" \
    "        SUM(ISNULL(FeesOut, 0)) AS FeesOut, SUM(ISNULL(Wtd_FeesOut, 0)) AS Wtd_FeesOut, \n" \
    "        SUM(ISNULL(Transfers, 0)) AS Transfers, SUM(ISNULL(TransferIn, 0)) AS TransferIn, \n" \
    "        SUM(ISNULL(TransferOut, 0)) AS TransferOut, SUM(ISNULL(EstAnnIncome, 0)) AS EstAnnIncome, \n" \
    "        SUM(ISNULL(NotionalFlow, 0)) AS NotionalFlow, SUM(ISNULL(Cons_Fee, 0)) AS Cons_Fee, \n" \
    "        SUM(ISNULL(Wtd_Cons, 0)) AS Wtd_Cons \n" \
    "			 FROM Merge_SData sd WITH (NOLOCK)  \n" \
    "			 JOIN Merge_CompSegMap sm WITH (NOLOCK)  ON sm.membersegid = sd.id \n" \
    "			 AND sm.SegmentType_ID = sd.SegmentTypeID \n" \
    "			 AND sm.SessionID = sd.SessionID \n" \
    "			 WHERE sd.SessionID = ? \n" \
    "			 AND perform_date > ? AND perform_date <= ? \n" \
    "			 GROUP BY sm.id \n" \
    "			) TTT \n" \
    "			JOIN Merge_CompSegMap smm WITH (NOLOCK)  ON smm.id = TTT.id \n" \
    "			AND smm.SessionID = ? \n\0"

void BuildSummarizeMonthsum_SQL(char *sSQL)
{
    strcpy_s(sSQL, MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sSQL, MAXSQLSIZE, SummarizeMonthsum_SQL_001);
    strcat_s(sSQL, MAXSQLSIZE, SummarizeMonthsum_SQL_002);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

// SummarizeInceptionSummdata
#define SummarizeInceptionSummdata_SQL_001 "INSERT INTO SUMMDATA WITH (ROWLOCK) \n" \
    "	SELECT smm.owner_id, TTT.* FROM ( \n" \
    "		SELECT MAX(sm.id) AS id, \n" \
    "			 ? AS Perform_date, \n" \
    "			CASE WHEN MAX(perform_date) = ? THEN 	\n" \
    "				SUM(CASE WHEN mkt_val < -1e+308 THEN 0 ELSE ISNULL(mkt_val,0) END) \n" \
    "			ELSE -1.7e+308 END AS Mkt_val, \n" \
    "        SUM(ISNULL(Book_value, 0)) AS Book_value, \n" \
    "        SUM(ISNULL(Accr_inc, 0)) AS Accr_inc, SUM(ISNULL(Accr_Div, 0)) AS Accr_Div, \n" \
    "        SUM(ISNULL(Inc_rclm, 0)) AS Inc_rclm, SUM(ISNULL(Div_rclm, 0)) AS Div_rclm, \n" \
    "        SUM(ISNULL(Net_Flow, 0)) AS Net_Flow, 0 AS Cum_Flow, \n" \
    "			 SUM(ISNULL(Wtd_flow, 0)) AS Wtd_flow, \n" \
    "        SUM(ISNULL(Purchases, 0)) AS Purchases, SUM(ISNULL(Sales, 0)) AS Sales, \n" \
    "        SUM(ISNULL(Income, 0)) AS Income, 0 AS Cum_income, \n" \
    "			 SUM(ISNULL(Wtd_inc, 0)) AS Wtd_inc, \n" \
    "        SUM(ISNULL(Fees, 0)) AS Fees, 0 AS Cum_fees, \n" \
    "			 SUM(ISNULL(Wtd_Fees, 0)) AS Wtd_Fees,  \n" \
    "        1 AS Exch_rate_base, 'MV' AS Interval_type, 0 AS Days_since_nond, 0 AS Days_since_Last, \n" \
    "        GETDATE() AS Create_Date, GETDATE() AS Change_Date, 'I' AS Perform_type,  \n" \
    "        SUM(ISNULL(PrincipalPayDown, 0)) AS PrincipalPayDown, SUM(ISNULL(Maturity, 0)) AS Maturity, \n\0"

#define SummarizeInceptionSummdata_SQL_002 "SUM(ISNULL(Contributions, 0)) AS Contributions, SUM(ISNULL(Withdrawals, 0)) AS Withdrawals, \n" \
    "        SUM(ISNULL(Expenses, 0)) AS Expenses, SUM(Receipts, 0) AS Receipts, \n" \
    "        SUM(ISNULL(IncomeCash, 0)) AS IncomeCash, SUM(ISNULL(PrincipalCash, 0)) AS PrincipalCash, \n" \
    "        SUM(ISNULL(FeesOut, 0)) AS FeesOut, SUM(ISNULL(Wtd_FeesOut, 0)) AS Wtd_FeesOut, \n" \
    "        SUM(ISNULL(Transfers, 0)) AS Transfers, SUM(ISNULL(TransferIn, 0)) AS TransferIn, \n" \
    "        SUM(ISNULL(TransferOut, 0)) AS TransferOut, SUM(ISNULL(EstAnnIncome, 0)) AS EstAnnIncome, \n" \
    "        SUM(ISNULL(NotionalFlow, 0)) AS NotionalFlow, SUM(ISNULL(Cons_Fee, 0)) AS Cons_Fee, \n" \
    "        SUM(ISNULL(Wtd_Cons, 0)) AS Wtd_Cons \n" \
    "			 FROM Merge_SData sd WITH (NOLOCK)  \n" \
    "			 JOIN Merge_CompSegMap sm WITH (NOLOCK)  ON sm.membersegid = sd.id \n" \
    "			 AND sm.SegmentType_ID = sd.SegmentTypeID \n" \
    "			 AND sm.SessionID = sd.SessionID \n" \
    "			 WHERE sd.SessionID = ? \n" \
    "			 AND perform_date > ? AND perform_date <= ? \n" \
    "			 GROUP BY sm.id \n" \
    "			) TTT \n" \
    "			JOIN Merge_CompSegMap smm WITH (NOLOCK)  ON smm.id = TTT.id \n" \
    "			AND smm.SessionID = ? \n\0"

void BuildSummarizeInceptionSummdata_SQL(char *sSQL)
{
    strcpy_s(sSQL, MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sSQL, MAXSQLSIZE, SummarizeInceptionSummdata_SQL_001);
    strcat_s(sSQL, MAXSQLSIZE, SummarizeInceptionSummdata_SQL_002);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}

// SubtractInceptionSummdata
#define SubtractInceptionSummdata_SQL_001 "UPDATE SUMMDATA WITH (ROWLOCK) SET \n" \
    "		Mkt_val = -1.7e+308, \n" \
    "        Book_value = SUMMDATA.Book_value - TTT.Book_value, \n" \
    "        Accr_inc = SUMMDATA.Accr_inc - TTT.Accr_inc, \n" \
    "		Accr_Div = SUMMDATA.Accr_Div - TTT.Accr_Div, \n" \
    "        Inc_rclm = SUMMDATA.Inc_rclm - TTT.Inc_rclm, \n" \
    "		Div_rclm = SUMMDATA.Div_rclm - TTT.Div_rclm, \n" \
    "        Net_Flow = SUMMDATA.Net_Flow - TTT.Net_Flow, \n" \
    "		Wtd_flow = SUMMDATA.Wtd_flow - TTT.Wtd_flow, \n" \
    "        Purchases = SUMMDATA.Purchases - TTT.Purchases, \n" \
    "		Sales = SUMMDATA.Sales - TTT.Sales, \n" \
    "        Income = SUMMDATA.Income - TTT.Income, \n" \
    "		Wtd_inc = SUMMDATA.Wtd_inc - TTT.Wtd_inc, \n" \
    "        Fees = SUMMDATA.Fees - TTT.Fees, \n" \
    "		Wtd_Fees = SUMMDATA.Wtd_Fees - TTT.Wtd_Fees, \n" \
    "        Change_Date = GETDATE(), \n" \
    "        PrincipalPayDown = SUMMDATA.PrincipalPayDown - TTT.PrincipalPayDown, \n" \
    "		Maturity = SUMMDATA.Maturity - TTT.Maturity, \n" \
    "		Contributions = SUMMDATA.Contributions - TTT.Contributions, \n" \
    "		Withdrawals = SUMMDATA.Withdrawals - TTT.Withdrawals, \n" \
    "        Expenses = SUMMDATA.Expenses - TTT.Expenses, \n" \
    "		Receipts = SUMMDATA.Receipts - TTT.Receipts, \n" \
    "        IncomeCash = SUMMDATA.IncomeCash - TTT.IncomeCash, \n" \
    "		PrincipalCash = SUMMDATA.PrincipalCash - TTT.PrincipalCash, \n" \
    "        FeesOut = SUMMDATA.FeesOut - TTT.FeesOut, \n" \
    "		Wtd_FeesOut = SUMMDATA.Wtd_FeesOut - TTT.Wtd_FeesOut, \n" \
    "        Transfers = SUMMDATA.Transfers - TTT.Transfers, \n" \
    "		TransferIn = SUMMDATA.TransferIn - TTT.TransferIn, \n" \
    "        TransferOut = SUMMDATA.TransferOut - TTT.TransferOut, \n" \
    "		EstAnnIncome = SUMMDATA.EstAnnIncome - TTT.EstAnnIncome, \n" \
    "        NotionalFlow = SUMMDATA.NotionalFlow - TTT.NotionalFlow, \n" \
    "		Cons_Fee = SUMMDATA.Cons_Fee - TTT.Cons_Fee, \n" \
    "        Wtd_Cons = SUMMDATA.Wtd_Cons - TTT.Wtd_Cons \n" \
    "	FROM ( \n" \
    "	SELECT smm.owner_id, TTT.* FROM ( \n" \
    "		SELECT MAX(sm.id) AS id, \n" \
    "			 ? AS Perform_date, \n" \
    "        SUM(ISNULL(Book_value, 0)) AS Book_value, \n" \
    "        SUM(ISNULL(Accr_inc, 0)) AS Accr_inc, SUM(ISNULL(Accr_Div, 0)) AS Accr_Div, \n" \
    "        SUM(ISNULL(Inc_rclm, 0)) AS Inc_rclm, SUM(ISNULL(Div_rclm, 0)) AS Div_rclm, \n" \
    "        SUM(ISNULL(Net_Flow, 0)) AS Net_Flow, \n" \
    "			 SUM(ISNULL(Wtd_flow, 0)) AS Wtd_flow, \n" \
    "        SUM(ISNULL(Purchases, 0)) AS Purchases, SUM(ISNULL(Sales, 0)) AS Sales, \n" \
    "        SUM(ISNULL(Income, 0)) AS Income, \n" \
    "			 SUM(ISNULL(Wtd_inc, 0)) AS Wtd_inc, \n" \
    "        SUM(ISNULL(Fees, 0)) AS Fees, \n" \
    "			 SUM(ISNULL(Wtd_Fees, 0)) AS Wtd_Fees,  \n" \
    "        SUM(ISNULL(PrincipalPayDown, 0)) AS PrincipalPayDown, SUM(ISNULL(Maturity, 0)) AS Maturity, \n\0"

#define SubtractInceptionSummdata_SQL_002 "SUM(ISNULL(Contributions, 0)) AS Contributions, SUM(ISNULL(Withdrawals, 0)) AS Withdrawals, \n" \
    "        SUM(ISNULL(Expenses, 0)) AS Expenses, SUM(Receipts, 0) AS Receipts, \n" \
    "        SUM(ISNULL(IncomeCash, 0)) AS IncomeCash, SUM(ISNULL(PrincipalCash, 0)) AS PrincipalCash, \n" \
    "        SUM(ISNULL(FeesOut, 0)) AS FeesOut, SUM(ISNULL(Wtd_FeesOut, 0)) AS Wtd_FeesOut, \n" \
    "        SUM(ISNULL(Transfers, 0)) AS Transfers, SUM(ISNULL(TransferIn, 0)) AS TransferIn, \n" \
    "        SUM(ISNULL(TransferOut, 0)) AS TransferOut, SUM(ISNULL(EstAnnIncome, 0)) AS EstAnnIncome, \n" \
    "        SUM(ISNULL(NotionalFlow, 0)) AS NotionalFlow, SUM(ISNULL(Cons_Fee, 0)) AS Cons_Fee, \n" \
    "        SUM(ISNULL(Wtd_Cons, 0)) AS Wtd_Cons \n" \
    "			 FROM Merge_SData sd WITH (NOLOCK)  \n" \
    "			 JOIN Merge_CompSegMap sm WITH (NOLOCK)  ON sm.membersegid = sd.id \n" \
    "			 AND sm.SegmentType_ID = sd.SegmentTypeID \n" \
    "			 AND sm.SessionID = sd.SessionID \n" \
    "			 WHERE sd.SessionID = ? \n" \
    "			 AND perform_date > ? AND perform_date <= ? \n" \
    "			 GROUP BY sm.id \n" \
    "			) TTT \n" \
    "			JOIN Merge_CompSegMap smm WITH (NOLOCK)  ON smm.id = TTT.id \n" \
    "			AND smm.SessionID = ? \n" \
    "	) TTT \n" \
    "	WHERE SUMMDATA.id = TTT.id AND SUMMDATA.Perform_date = ? AND SUMMDATA.Perform_Type = 'I' \n\0"

void BuildSubtractInceptionSummdata_SQL(char *sSQL)
{
    strcpy_s(sSQL, MAXSQLSIZE, ANY_SQL_000);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_UNCOMMITTED);
    strcat_s(sSQL, MAXSQLSIZE, SubtractInceptionSummdata_SQL_001);
    strcat_s(sSQL, MAXSQLSIZE, SubtractInceptionSummdata_SQL_002);
    strcat_s(sSQL, MAXSQLSIZE, SQL_SET_TRANS_ISOLATION_LVL_COMMITTED);
}
