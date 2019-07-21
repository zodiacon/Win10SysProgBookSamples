#pragma once

namespace wil {
	static void close_private_ns(HANDLE h) {
		::ClosePrivateNamespace(h, 0);
	};

	using unique_private_ns = unique_any_handle_null_only<decltype(&close_private_ns), close_private_ns>;

	using unique_bound_desc = unique_any_handle_null_only<decltype(&::DeleteBoundaryDescriptor), ::DeleteBoundaryDescriptor>;
}
