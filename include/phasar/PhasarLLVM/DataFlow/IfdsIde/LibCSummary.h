#pragma once

namespace psr {
namespace library_summary {
class FunctionDataFlowFacts;
} // namespace library_summary

[[nodiscard]] const library_summary::FunctionDataFlowFacts &getLibCSummary();
} // namespace psr
