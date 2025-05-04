/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DOMAIN_ANALYSISDOMAIN_H
#define PHASAR_DOMAIN_ANALYSISDOMAIN_H

#include <type_traits>

namespace psr {

// AnalysisDomain - This class should be specialized by different static
// analyses types... which is why the default version declares all analysis
// domains as aliases of void.
//
// Virtually all of PhASAR's internal analyses are implemented in a generic way
// using interfaces and template parameters. In order to specify concrete types
// for the template parameters such that an analysis can compute some useful
// information on some concrete target code, a configuration template parameter
// of type AnalysisDomain is passed around to make the necessary information
// available to the required analyses.
//
// If a type is not meant to be used by an analysis it should be left as an
// alias to void. If any analysis detects that a parameter is required to
// conduct an analysis but not correctly set, it will statically report an error
// and ask for the missing piece of information.
struct AnalysisDomain {
  // Data-flow fact --- Specifies the type of an individual data-flow fact that
  // is propagated through the program under analysis.
  using d_t = void;
  // (Control-flow) Node --- Specifies the type of a node in the
  // (inter-procedural) control-flow graph and can be though of as an individual
  // statement or instruction of the program.
  using n_t = void;
  // Function --- Specifies the type of functions.
  using f_t = void;
  // (User-defined) type --- Specifies the type of a user-defined (i.e. struct
  // or class) data type.
  using t_t = void;
  // (Pointer) value --- Specifies the type of pointers.
  using v_t = void;
  // Intra-procedural control flow --- Specifies the type of the
  // control-flow graph to be used.
  using c_t = void;
  // Inter-procedural control flow --- Specifies the type of the
  // inter-procedural control-flow graph to be used.
  using i_t = void;
  // The ProjectIRDB type to use. Must inherit from the ProjectIRDBBase CRTP
  // template
  using db_t = void;
  // Lattice element --- Specifies the type of the underlying lattice; the value
  // computation domain IDE's edge functions or WPDS's weights operate on.
  using l_t = void;
  // Container type to be used for analyses run in the monotone framework.
  using mono_container_t = void;
};

enum class BinaryDomain;

namespace detail {
template <typename AnalysisDomainTy, typename ValueTy = BinaryDomain>
struct HasBinaryValueDomain : std::false_type {};
template <typename AnalysisDomainTy>
struct HasBinaryValueDomain<AnalysisDomainTy, typename AnalysisDomainTy::l_t>
    : std::true_type {};

template <typename AnalysisDomainTy>
struct WithBinaryValueDomainExtender : AnalysisDomainTy {
  using l_t = BinaryDomain;
};

} // namespace detail

template <typename AnalysisDomainTy>
using WithBinaryValueDomain =
    std::conditional_t<detail::HasBinaryValueDomain<AnalysisDomainTy>::value,
                       AnalysisDomainTy,
                       detail::WithBinaryValueDomainExtender<AnalysisDomainTy>>;

} // namespace psr

#endif
