#include "stdafx.h"
#include "SortHelper.h"

bool SortHelper::SortStrings(const CString& s1, const CString& s2, bool ascending) {
	return ascending ? s2.CompareNoCase(s1) > 0 : s2.CompareNoCase(s1) < 0;
}

bool SortHelper::SortStrings(PCWSTR s1, PCWSTR s2, bool ascending) {
	auto compare = ::_wcsicmp(s2, s1);
	return ascending ? compare > 0 : compare < 0;
}

bool SortHelper::SortBoolean(bool a, bool b, bool asc) {
	return asc ? b > a : a > b;
}
