#include "DigenerateQueries.h"
#include <stdio.h>
#include <string.h>
#include <memory>

// ============================================================================
// Simple Query Strings
// ============================================================================

const char* SQL_InsertAccdiv = 
    "INSERT INTO accdiv \
    (id, trans_no, divint_no, tran_type, \
    sec_no, wi, secid, sec_xtend, acct_type, \
    elig_date, sec_symbol,  \
    div_type, div_factor, units, orig_face, pcpl_amt, \
    income_amt, trd_date, stl_date, eff_date, entry_date, \
    curr_id, curr_acct_type, inc_curr_id, inc_acct_type, sec_curr_id, accr_curr_id, \
    base_xrate, inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, \
    inc_sys_xrate, orig_yld, eff_mat_date, eff_mat_price, acct_mthd, \
    trans_srce, dr_cr, dtc_inclusion, dtc_resolve, income_flag, \
    letter_flag, ledger_flag, created_by, create_date, create_time, \
    suspend_flag, delete_flag) \
    VALUES (?, ?, ?, ?,        ?, ?, ?, ?, ?,         ?, ?,  \
            ?, ?, ?, ?, ?,     ?, ?, ?, ?, ?,         ?, ?, ?, ?, ?, ?, \
            ?, ?, ?, ?, ?,     ?, ?, ?, ?, ?,         ?, ?, ?, ?, ?, \
            ?, ?, ?, ?, ?,     ?, ?) ";

const char* SQL_SelectOneAccdiv = 
    "SELECT id, trans_no, divint_no, tran_type, sec_no, wi, \
    secid, sec_xtend, acct_type, elig_date, sec_symbol, \
    div_type, div_factor, units, orig_face, pcpl_amt, \
    income_amt, trd_date, stl_date, eff_date, entry_date, \
    curr_id, curr_acct_type, inc_curr_id, inc_acct_type, \
    sec_curr_id, accr_curr_id, base_xrate, inc_base_xrate, \
    sec_base_xrate, accr_base_xrate, sys_xrate, inc_sys_xrate, \
    orig_yld, eff_mat_date, eff_mat_price, acct_mthd, trans_srce, \
    dr_cr, dtc_inclusion, dtc_resolve, income_flag, letter_flag, \
    ledger_flag, created_by, create_date, create_time, \
    suspend_flag, delete_flag  \
    FROM accdiv \
    WHERE id = ? AND divint_no = ? AND trans_no = ? ";

const char* SQL_UpdateAccdiv = 
    "UPDATE accdiv  \
    SET div_type = ?, div_factor = ?, pcpl_amt = ?, income_amt = ?, \
        trd_date = ?, stl_date = ?, eff_date = ? \
    WHERE id = ? AND divint_no = ? AND trans_no = ? ";

const char* SQL_SelectPartCurrency = 
    "SELECT c.curr_id, a.sec_no, a.when_issue, a.cur_exrate \
     FROM currency c, assets a \
     WHERE c.sec_no = a.sec_no AND \
     c.wi = a.when_issue";

const char* SQL_InsertDivhist = 
    "INSERT INTO divhist \
    (ID, Trans_No, Divint_No, Tran_Type, \
    Sec_No, Wi, SecID, Sec_Xtend, Acct_Type, \
    Units, Div_Trans_No, \
    Tran_Location, Ex_Date, Pay_Date) \
    VALUES (?, ?, ?, ?,		?, ?, ?, ?, ?, \
            ?, ?,			?, ?, ?)";

const char* SQL_UpdateDivhist = 
    "UPDATE divhist \
    SET units = ?, ex_date = ?, pay_date = ? \
    WHERE id = ? AND divint_no = ? AND trans_no = ? ";

const char* SQL_DeleteDivhist = 
    "DELETE FROM divhist \
    WHERE id = ? AND divint_no = ? AND trans_no = ? ";

const char* SQL_DeleteFWTrans = 
    "Delete From fwtrans Where id = ?";

const char* SQL_InsertFWTrans = 
    "INSERT INTO fwtrans \
    (id, trans_no, divint_no, tran_type, \
    sec_no, wi, secid, sec_xtend, acct_type, \
    div_type, div_factor, units, pcpl_amt, \
    income_amt, trd_date, stl_date, eff_date, \
    curr_id, inc_curr_id, inc_acct_type, sec_curr_id, accr_curr_id, \
    base_xrate, inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, \
    inc_sys_xrate, dr_cr, create_date, create_time, description) \
    VALUES (?, ?, ?, ?,        ?, ?, ?, ?, ?,         ?, ?, ?, ?, \
            ?, ?, ?, ?,        ?, ?, ?, ?, ?,         ?, ?, ?, ?, ?, \
            ?, ?, ?, ?, ?) ";

const char* SQL_SelectAllPartPortmain = 
    "SELECT P.Id, p.unique_name, p.FiscalYearEndMonth, p.FiscalYearEndDay, \
    p.InceptionDate, p.AcctMethod, p.BaseCurrId, p.Income, \
    p.Actions, p.Mature, p.CAvail, p.FAvail, p.ValDate, \
    p.DeleteDate, p.IsInactive, p.AmortMuni, \
    p.AmortOther, p.AmortStartDate, p.IncByLot, \
    p.PurgeDate, p.RollDate, p.Tax, p.PortfolioType, p.PricingEffectiveDate, \
    p.MaxEqPct, p.MaxFiPct, i.DefaultVendorID, i.ReturnsToCalculate, p.DefaultReturnType, \
    case when mi.id is null then 'N' else 'Y' end, i.AccretMuni, i.AccretOther \
    FROM Portmain as P \
    LEFT JOIN PortInfo as I on I.Id = P.ID \
    LEFT JOIN MktMain as mi on mi.id = p.id \
    WHERE p.PortfolioType in (3, 4, 5) \
    AND (ISNULL(p.DeleteDate, '12/30/1899') = '12/30/1899') \
    ORDER BY p.id";

const char* SQL_SelectOnePartPortmain = 
    "SELECT p.Id, p.unique_name, p.FiscalYearEndMonth, p.FiscalYearEndDay, \
    p.InceptionDate, p.AcctMethod, p.BaseCurrId, p.Income, \
    p.Actions, p.Mature, p.CAvail, p.FAvail, p.ValDate, \
    p.DeleteDate, p.IsInactive, p.AmortMuni, \
    p.AmortOther, p.AmortStartDate, p.IncByLot, \
    p.PurgeDate, p.RollDate, p.Tax, p.PortfolioType, p.PricingEffectiveDate, \
    p.MaxEqPct, p.MaxFiPct, i.DefaultVendorid, i.returnstocalculate, p.defaultreturntype, \
    case when mi.id is null then 'N' else 'Y' end, i.AccretMuni,i.AccretOther \
    FROM Portmain as P \
    LEFT JOIN PortInfo as I on I.Id = P.ID \
    LEFT JOIN MktMain as mi on mi.id = p.id \
    WHERE p.id = ?";

const char* SQL_SelectAllSubacct = 
    "SELECT acct_type, xref_acct_type \
    FROM subacct";

const char* SQL_SelectPortfolioRange = 
    "Select id from portmain where portfoliotype in (3, 4, 5) \
    and (ISNULL(deletedate, '12/30/1899') = '12/30/1899') \
    order by id ";

// ============================================================================
// Complex Query Builders
// ============================================================================

#define DIVINT_UNLOAD_1_PAID	"SELECT 'H' table_name, h.id p1, h.sec_no p2, h.wi p3, h.sec_xtend p4, \n\
				h.sec_symbol, h.acct_type p5, h.trans_no p6, h.units, h.eff_date, h.elig_date, \n\
				h.stl_date,h.orig_face, h.orig_yield, h.eff_mat_date, h.eff_mat_price, \n\
				h.orig_trans_type, h.trd_date p7, h.safek_ind, h.trans_no p8, \n\
				a.id secid, a.trad_unit, a.sec_type, a.auto_action, a.auto_divint, \n\
				a.curr_id, a.inc_curr_id, a.cur_exrate, a.cur_inc_exrate, \n\
				di.divint_no, di.div_type, di.ex_date, di.rec_date, di.pay_date, \n\
				di.div_rate, di.post_status,di.create_date, di.modify_date, \n\
				di.fwd_divint_no, di.prev_divint_no, di.delete_flag, \n\
				dt.process_flag, dt.process_type, dt.short_settle, dt.incl_units, dt.split_ind,   \n\
				dh.tran_type, dh.div_trans_no, dh.tran_location, dt.tran_type, h.trd_date p9 \n\
		FROM ((( %HOLDINGS_TABLE_NAME% h \n\
		INNER JOIN assets a on h.sec_no = a.sec_no and h.wi = a.when_issue)\n\
        INNER JOIN divint di on h.sec_no = di.sec_no AND h.wi = di.wi) \n\
        INNER JOIN divtypes dt on di.div_type = dt.div_type)	\n\
		JOIN divhist dh on h.id = dh.id AND dh.trans_no = h.trans_no and dh.divint_no = di.divint_no, \n\
		Portmain p  \n\
		WHERE	h.sec_xtend != 'TS' AND h.sec_xtend != 'TL' AND \n\
				di.div_rate != 0 AND \n\
				dt.process_flag = 'Y' AND \n\
				((a.auto_divint = 'Y' AND (dt.process_type in ('I', 'L', 'C'))) OR \n\
				(a.auto_action = 'Y' AND (dt.process_type in ('S', 'D')))) AND \n\
				di.ex_date >= ? AND di.ex_date <= ? AND \n\
				(((dt.process_type in ('C','I','L')) AND \n\
				(h.elig_date < di.ex_date OR \n\
				(h.stl_date <= di.rec_date AND	\n\
				dt.short_settle = 'Y'))) OR \n\
				((dt.process_type in ('S','D')) \n\
				AND h.elig_date <= di.pay_date)) \n\
				AND h.id = p.id \0"

#define DIVINT_UNLOAD_1_TOPAY	"UNION ALL \n\
				SELECT 'H' table_name, h.id p1, h.sec_no p2, h.wi p3, h.sec_xtend p4, \n\
				h.sec_symbol, h.acct_type p5, h.trans_no p6, h.units, h.eff_date, h.elig_date, \n\
				h.stl_date,h.orig_face, h.orig_yield, h.eff_mat_date, h.eff_mat_price, \n\
				h.orig_trans_type, h.trd_date p7, h.safek_ind, h.trans_no p8, \n\
				a.id secid, a.trad_unit, a.sec_type, a.auto_action, a.auto_divint, \n\
				a.curr_id, a.inc_curr_id, a.cur_exrate, a.cur_inc_exrate, \n\
				di.divint_no, di.div_type, di.ex_date, di.rec_date, di.pay_date, \n\
				di.div_rate, di.post_status,di.create_date, di.modify_date, \n\
				di.fwd_divint_no, di.prev_divint_no, di.delete_flag, \n\
				dt.process_flag, dt.process_type, dt.short_settle, dt.incl_units, dt.split_ind,   \n\
				NULL, NULL, NULL, dt.tran_type, h.trd_date p9 \n\
		FROM ((( %HOLDINGS_TABLE_NAME% h \n\
		INNER JOIN assets a on h.sec_no = a.sec_no and h.wi = a.when_issue)\n\
        INNER JOIN divint di on h.sec_no = di.sec_no AND h.wi = di.wi) \n\
        INNER JOIN divtypes dt on di.div_type = dt.div_type),	\n\
		Portmain p  \n\
		WHERE	NOT EXISTS (SELECT * FROM divhist dh \n\
			WHERE h.id = dh.id AND dh.trans_no = h.trans_no AND dh.divint_no =  di.divint_no) \n\
		AND		h.sec_xtend != 'TS' AND h.sec_xtend != 'TL' AND \n\
				di.div_rate != 0 AND \n\
				dt.process_flag = 'Y' AND \n\
				((a.auto_divint = 'Y' AND (dt.process_type in ('I','L','C'))) OR \n\
				(a.auto_action = 'Y' AND (dt.process_type in ('S','D')))) AND \n\
				di.ex_date >= ? AND di.ex_date <= ? AND \n\
				(((dt.process_type in ('C', 'I', 'L')) AND \n\
				(h.elig_date < di.ex_date OR \n\
				(h.stl_date <= di.rec_date AND	\n\
				dt.short_settle = 'Y'))) OR \n\
				((dt.process_type in ('S','D')) \n\
				AND h.elig_date <= di.pay_date)) \n\
				AND h.id = p.id \0"

#define DIVINT_UNLOAD_TYPE_I "AND (dt.process_type in ('I','C','L')) \0"

#define DIVINT_UNLOAD_TYPE_S "AND (dt.process_type in ('S','D')) \0"

#define DIVINT_UNLOAD_1_MODE_B	" and h.id = p.id and p.portfoliotype in (3,4,5) and h.id >? \
								and h.id <=? and	\
								(ISNULL(p.deleteDate, '12/30/1899') = '12/30/1899') \0"

#define DIVINT_UNLOAD_1_MODE_A	" AND h.id = ? \0"


#define DIVINT_UNLOAD_1_MODE_S	" AND h.id = ? AND h.sec_no = ? AND h.wi = ? AND \
								h.sec_xtend = ? AND h.acct_type = ? \0"

#define DIVINT_UNLOAD_2	"UNION ALL \
	   SELECT 'D' table_name, hd.id p1, hd.sec_no p2, hd.wi p3, hd.sec_xtend p4, \
     hd.sec_symbol, hd.acct_type p5, hd.trans_no p6, hd.units, hd.eff_date, hd.elig_date, \
     hd.stl_date, hd.orig_face, hd.orig_yield, hd.eff_mat_date, hd.eff_mat_price, \
     hd.orig_trans_type, t.stl_date p7, hd.safek_ind, hd.create_trans_no p8, \
     a.id secid, a.trad_unit, a.sec_type, a.auto_action, a.auto_divint, \
     a.curr_id, a.inc_curr_id, a.cur_exrate, a.cur_inc_exrate, \
     di.divint_no, di.div_type, di.ex_date, di.rec_date, di.pay_date, \
     di.div_rate, di.post_status, di.create_date,di.modify_date, \
     di.fwd_divint_no, di.prev_divint_no, di.delete_flag, \
     dt.process_flag, dt.process_type, dt.short_settle, dt.incl_units, dt.split_ind,  \
     dh.tran_type, dh.div_trans_no, dh.tran_location, dt.tran_type, hd.create_date p9 \
        FROM (((holddel hd \
				INNER JOIN assets a ON hd.sec_no = a.sec_no AND hd.wi = a.when_issue ) \
				INNER JOIN divint di ON hd.sec_no = di.sec_no AND hd.wi = di.wi ) \
				INNER JOIN divtypes dt ON di.div_type = dt.div_type ) \
				LEFT OUTER JOIN divhist dh ON hd.id = dh.id AND dh.trans_no = hd.trans_no \
															AND dh.divint_no = di.divint_no, \
				portmain p, trans t \
        WHERE  hd.rev_trans_no = 0 AND t.stl_date >= di.rec_date AND \
               hd.sec_xtend != 'TS' AND hd.sec_xtend != 'TL' AND \
               hd.rev_trans_no = 0 and di.div_rate != 0 \
               AND (hd.id = t.id and hd.create_trans_no = t.trans_no)  \
               AND dt.process_flag = 'Y' AND \
               dt.process_type != 'S' AND \
							 ((a.auto_divint = 'Y' AND (dt.process_type in ('I','L','C'))) OR \
							 (a.auto_action = 'Y' AND (dt.process_type in ('S','D')))) AND \
               di.ex_date >= ? AND di.ex_date <= ? AND \
               (hd.elig_date < di.ex_date OR \
               (hd.stl_date <= di.rec_date AND \
               dt.short_settle = 'Y')) \
               AND hd.id = p.id \0"

#define DIVINT_UNLOAD_2_MODE_B	" and hd.id = p.id and p.portfoliotype in (3,4,5) and hd.id >? \
								and hd.id <=? and	\
								(ISNULL(p.deleteDate, '12/30/1899') = '12/30/1899') \0"

#define DIVINT_UNLOAD_2_MODE_A	" AND hd.id = ? \0"


#define DIVINT_UNLOAD_2_MODE_S	" AND hd.id = ? AND hd.sec_no = ? AND hd.wi = ? AND \
								hd.sec_xtend = ? AND hd.acct_type = ? \0"

#define DIVINT_UNLOAD_END		"Order by p1, p2, p3, p4, p5, di.divint_no, p6, table_name, p9, p8 \0"

// Helper function to replace %HOLDINGS_TABLE_NAME% with sHoldings
void AdjustSQL(char* sSQL, const char* sHoldings)
{
    char* p = strstr(sSQL, "%HOLDINGS_TABLE_NAME%");
    if (p)
    {
        // Allocate sTemp on the heap to reduce stack usage
        std::unique_ptr<char[]> sTemp(new char[MAXSQLSIZE]);
        strcpy_s(sTemp.get(), MAXSQLSIZE, p + strlen("%HOLDINGS_TABLE_NAME%"));
        strcpy_s(p, MAXSQLSIZE - (p - sSQL), sHoldings);
        strcat_s(sSQL, MAXSQLSIZE, sTemp.get());
    }
}

void BuildDivintUnloadSQL(char* sAdjSQL, const char* sHoldings, const char* sType, char cMode)
{
    strcpy_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_PAID);
    AdjustSQL(sAdjSQL, sHoldings);

    switch (sType[0])
    {
        case 'S':
            strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_TYPE_S);
            break;
        case 'I':
            strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_TYPE_I);
            break;
    }
    
    switch (cMode)
    {
        case 'B': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_MODE_B); break;
        case 'A': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_MODE_A); break;
        case 'S': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_MODE_S); break;
    }

    strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_TOPAY);
    AdjustSQL(sAdjSQL, sHoldings);

    switch (sType[0])
    {
        case 'S':
            strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_TYPE_S);
            break;
        case 'I':
            strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_TYPE_I);
            break;
    }
    
    switch (cMode)
    {
        case 'B': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_MODE_B); break;
        case 'A': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_MODE_A); break;
        case 'S': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_1_MODE_S); break;
    }

    strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_2);

    switch (sType[0])
    {
        case 'S':
            strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_TYPE_S);
            break;
        case 'I':
            strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_TYPE_I);
            break;
    }
    
    switch (cMode)
    {
        case 'B': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_2_MODE_B); break;
        case 'A': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_2_MODE_A); break;
        case 'S': strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_2_MODE_S); break;
    }

    strcat_s(sAdjSQL, MAXSQLSIZE, DIVINT_UNLOAD_END);
}
