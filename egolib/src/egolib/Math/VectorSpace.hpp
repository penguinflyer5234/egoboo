#pragma once

#include "egolib/Math/Dimensionality.hpp"
#include "egolib/Math/ScalarField.hpp"

namespace Ego {
namespace Math {

namespace Internal {

/**
 * @brief
 *  A struct derived from @a std::true_type if @a _Dimensionality fulfils the
 *  properties of a <em>dimensionality  concept</em> and derived from
 *  @a std::false_type otherwise. Furthermore, @a _ScalarFieldType shall be a
 *	be of type <tt>Ego::Math::ScalarField</tt>.
 */
template <typename _ScalarFieldType, size_t _Dimensionality>
struct VectorSpaceEnable
	: public std::conditional <
	  (IsDimensionality<_Dimensionality>::value),
	  std::true_type,
	  std::false_type
	  >::type
{};


} // namespace Internal

/**
 * @brief
 *	A line.
 * @remark
 *	A line is defined as \f$L(t) = O + t \vec{d}\f$ where \f$P\f$ is an origin point,
 *	\f$\vec{d}\f$ is a unit-length direction vector, and \f$t\f$ is any real number.
 * @author
 *	Michael Heilmann
 */
template <typename _ScalarFieldType, size_t _Dimensionality, 
		  typename _Enabled = void>
struct VectorSpace;

template <typename _ScalarFieldType, size_t _Dimensionality>
struct VectorSpace<_ScalarFieldType, _Dimensionality, 
	               typename std::enable_if<Internal::VectorSpaceEnable<_ScalarFieldType, _Dimensionality>::value>::type> {
	
	/**
	 * @invariant
	 *	@a _Dimensionality must fulfil the <em>dimensionality concept</em>.
	 */
	static_assert(IsDimensionality<_Dimensionality>::value, "_Dimensionality must fulfil the dimensionality concept");

	/**
	 * @brief
	 *	The scalar type.
	 */
	typedef typename _ScalarFieldType::ScalarType ScalarType;
	/**
	 * @brief
	 *	The scalar field type.
	 */
	typedef _ScalarFieldType ScalarFieldType;

#if !defined(_MSC_VER)
	/**
	 * @brief
	 *	The dimensionality of the vector space.
	 * @return
	 *	the dimensionality of the vector space
	 */
	constexpr static size_t dimensionality() {
		return _Dimensionality;
	}
#endif

	/**
	 * @brief
	 *	The dimensionality of the vector space.
	 * @todo
	 *	Should be a constexpr function. Not yet possible because of - guess what - MSVC.
	 */
	typedef typename std::integral_constant < size_t, _Dimensionality > Dimensionality;
};

#if 0
template <typename _ScalarFieldType, size_t _Dimensionality>
const size_t VectorSpace<_ScalarFieldType, _Dimensionality, typename std::enable_if<Internal::VectorSpaceEnable<_ScalarFieldType, _Dimensionality>::value>::type>::Dimensionality = _Dimensionality;
#endif

} // namespace Math
} // namespace Ego
