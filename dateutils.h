#pragma once
#include "nanodbc/nanodbc.h"
#include <string>
#include <cstring>  // Для strncpy_s

// Helper to convert nanodbc::date to long (Delphi date: days since 1899-12-30)
inline long date_to_long(const nanodbc::date& d) {
    if (d.year == 0) return 0;  // NULL/Invalid
    // Julian day number (JDN) calculation
    int a = (14 - d.month) / 12;
    int y = d.year + 4800 - a;
    int m = d.month + 12 * a - 3;
    long jdn = d.day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    // OLE/Delphi date offset from JDN (to 1899-12-30)
    return jdn - 2415019;  // Adjust if time component needed (here date-only)
}

// Helper to convert nanodbc::timestamp to long (truncate time or include fraction)
inline long timestamp_to_long(const nanodbc::timestamp& ts) {
    if (ts.year == 0) return 0;
    // Use date part for JDN
    nanodbc::date datePart = { ts.year, ts.month, ts.day };
    long days = date_to_long(datePart);
    // Add fractional day for time (if needed; else return days)
    double frac = (ts.hour / 24.0) + (ts.min / 1440.0) + (ts.sec / 86400.0) + (ts.fract / 86400000000000.0);  // fraction in nanoseconds
    return static_cast<long>(days + frac);  // Truncate or round as per app logic
}

// Helper to convert long (Delphi/OLE date: days since 1899-12-30) to nanodbc::timestamp
inline void long_to_timestamp(long lDate, nanodbc::timestamp& ts) {
    if (lDate == 0) {
        ts.year = 0; ts.month = 0; ts.day = 0;
        ts.hour = 0; ts.min = 0; ts.sec = 0; ts.fract = 0;
        return;
    }
    // Convert OLE/Delphi date to JDN
    long jdn = lDate + 2415019;
    // Julian to Gregorian conversion
    int a = jdn + 32044;
    int b = (4 * a + 3) / 146097;
    int c = a - (146097 * b) / 4;
    int d = (4 * c + 3) / 1461;
    int e = c - (1461 * d) / 4;
    int m = (5 * e + 2) / 153;
    
    ts.day = e - (153 * m + 2) / 5 + 1;
    ts.month = m + 3 - 12 * (m / 10);
    ts.year = 100 * b + d - 4800 + m / 10;
    ts.hour = 0;
    ts.min = 0;
    ts.sec = 0;
    ts.sec = 0;
    ts.fract = 0;
}

// Helper to convert long to nanodbc::date (wrapper for compatibility)
inline nanodbc::date LongToDateStruct(long lDate) {
    nanodbc::timestamp ts;
    long_to_timestamp(lDate, ts);
    return {ts.year, ts.month, ts.day};
}

// Helper to convert nanodbc::date to long (wrapper for compatibility)
inline long DateStructToLong(const nanodbc::date& d) {
    return date_to_long(d);
}

// Helper to read string by column name (safe, with null/empty handling)
inline void read_string(const nanodbc::result& result, const std::string& col_name, char* dest, size_t len) {
    try {
        if (result.is_null(col_name)) {
            dest[0] = '\0';
        }
        else {
            std::string tmp = result.get<std::string>(col_name);
            strncpy_s(dest, len, tmp.c_str(), _TRUNCATE);
        }
    }
    catch (...) {
        dest[0] = '\0';
    }
}

// Helper for dates (handles DATE/TIMESTAMP, outputs long for Delphi OLE date; by name)
inline void read_date(const nanodbc::result& result, const std::string& col_name, long* dest) {
    try {
        if (result.is_null(col_name)) {
            *dest = 0;
        }
        else {
            // Try timestamp first (for DATETIME)
            nanodbc::timestamp ts = result.get<nanodbc::timestamp>(col_name);
            *dest = timestamp_to_long(ts);
        }
    }
    catch (const nanodbc::type_incompatible_error&) {
        // Fallback to date (for pure DATE columns)
        nanodbc::date d = result.get<nanodbc::date>(col_name);
        *dest = date_to_long(d);
    }
    catch (...) {
        *dest = 0;
    }
}

// Overload to return long directly
inline long read_date(const nanodbc::result& result, const std::string& col_name) {
    long val = 0;
    read_date(result, col_name, &val);
    return val;
}

// Helper for doubles (0.0 if null/error)
inline void read_double(const nanodbc::result& result, const std::string& col_name, double* dest) {
    try {
        if (result.is_null(col_name)) {
            *dest = 0.0;
        }
        else {
            *dest = result.get<double>(col_name);
        }
    }
    catch (...) {
        *dest = 0.0;
    }
}

// Helper for ints (0 if null/error)
inline void read_int(const nanodbc::result& result, const std::string& col_name, int* dest) {
    try {
        if (result.is_null(col_name)) {
            *dest = 0;
        }
        else {
            *dest = result.get<int>(col_name);
        }
    }
    catch (...) {
        *dest = 0;
    }
}
// Helper to read a long (int64/long) from nanodbc::result by column name
inline void read_long(const nanodbc::result& result, const char* col, long* out)
{
    try
    {
        // nanodbc::result::get<T>(string) throws if null, so fallback to 0
        *out = result.get<long>(col, 0L);
    }
    catch (...)
    {
        *out = 0L;
    }
}
inline void read_short(const nanodbc::result& result, const std::string& col_name, short* dest) {
    if (result.is_null(col_name)) {
        *dest = 0;
    } else {
        *dest = static_cast<short>(result.get<int>(col_name));
    }
}

// Index-based overloads
inline void read_string(const nanodbc::result& result, int col, char* dest, size_t len) {
    try {
        if (result.is_null(col)) {
            dest[0] = '\0';
        }
        else {
            std::string tmp = result.get<std::string>(col);
            strncpy_s(dest, len, tmp.c_str(), _TRUNCATE);
        }
    }
    catch (...) {
        dest[0] = '\0';
    }
}

inline void read_date(const nanodbc::result& result, int col, long* dest) {
    try {
        if (result.is_null(col)) {
            *dest = 0;
        }
        else {
            nanodbc::timestamp ts = result.get<nanodbc::timestamp>(col);
            *dest = timestamp_to_long(ts);
        }
    }
    catch (const nanodbc::type_incompatible_error&) {
        nanodbc::date d = result.get<nanodbc::date>(col);
        *dest = date_to_long(d);
    }
    catch (...) {
        *dest = 0;
    }
}

// Overload to return long directly (index)
inline long read_date(const nanodbc::result& result, int col) {
    long val = 0;
    read_date(result, col, &val);
    return val;
}

inline void read_double(const nanodbc::result& result, int col, double* dest) {
    try {
        if (result.is_null(col)) {
            *dest = 0.0;
        }
        else {
            *dest = result.get<double>(col);
        }
    }
    catch (...) {
        *dest = 0.0;
    }
}

inline void read_int(const nanodbc::result& result, int col, int* dest) {
    try {
        if (result.is_null(col)) {
            *dest = 0;
        }
        else {
            *dest = result.get<int>(col);
        }
    }
    catch (...) {
        *dest = 0;
    }
}

inline void read_short(const nanodbc::result& result, int col, short* dest) {
    if (result.is_null(col)) {
        *dest = 0;
    } else {
        *dest = static_cast<short>(result.get<int>(col));
    }
}
