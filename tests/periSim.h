//
//
// MIT License
//
// Copyright (c) 2020 Stellacore Corporation.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
// KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
// IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//


#ifndef peri_Sim_INCL_
#define peri_Sim_INCL_


#include "peridetic.h"

#include "periLocal.h"

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>


namespace peri::sim
{
	//! Closed interval of values (first/last both included)
	using Range = std::pair<double, double>;

	//! Multiplier to approximation "just inside of" open interval end point
	static double const sEndFrac{ 1. - std::numeric_limits<double>::epsilon() };

	//! Longitude range - approximate the half open interval [-pi,pi)
	static Range const sRangeLon{ -peri::pi(), sEndFrac*peri::pi() };

	//! Parallels (latitude) range - treat as closed interval [-pi/2,pi/2]
	static Range const sRangePar{ -.5*peri::pi(), .5*peri::pi() };


	//! Geometric sampling relationships for uniformly space sampling
	struct SampleSpec
	{
		std::size_t const theNumSamps{};
		std::pair<double, double> const theRange{};
		double theDelta{};

		//! Increment producing number of samples spanning *CLOSED* range
		inline
		static
		double
		deltaFor
			( std::size_t const & numSamps
			, std::pair<double, double> const & range
			)
		{
			double const rangeMag{ range.second - range.first };
			double delta{ 0. }; // case for only one sample
			if (1u < numSamps)
			{
				delta = (1./static_cast<double>(numSamps-1)) * rangeMag;
			}
			return delta;
		}

		//! Compute spacing associated with requested sampling
		inline
		explicit
		SampleSpec
			( std::size_t const & numSamps
			, std::pair<double, double> const & range
			)
			: theNumSamps{ numSamps }
			, theRange{ range }
			, theDelta{ deltaFor(theNumSamps, theRange) }
		{ }

		//! Number of samples generated by this spec
		inline
		std::size_t
		size
			() const
		{
			return theNumSamps;
		}

		//! Increment producing number of samples spanning *CLOSED* range
		inline
		double
		delta
			() const
		{
			return theDelta;
		}

		//! The first value that is *IN*cluded - i.e. start of range
		inline
		double
		first
			() const
		{
			return theRange.first;
		}

		//! The last value that is *IN*cluded - i.e. end of range
		inline
		double
		last
			() const
		{
			return theRange.second;
		}

		//! Value associated with sampling index (\note NO checking on ndx)
		inline
		double
		valueAtIndex
			( std::size_t const & ndx
			) const
		{
			return first() + static_cast<double>(ndx) * delta();
		}

	}; // SampleSpec

	inline
	static
	std::vector<double>
	samplesAccordingTo
		( SampleSpec const & spec
		)
	{
		std::vector<double> samps;
		samps.reserve(spec.size());
		for (std::size_t ndx{0u} ; ndx < spec.size() ; ++ndx)
		{
			double const value{ spec.valueAtIndex(ndx) };
			samps.emplace_back(value);
		}
		return samps;
	}

	//! Samples in meridian plane (distributed circularly (not geodetically))
	std::vector<XYZ>
	meridianPlaneSamples
		( SampleSpec const & radSpec
		, SampleSpec const & parSpec
		, double const & lonVal = .25*pi() 
			//!< generate plane at this longitude
		)
	{
		std::vector<XYZ> xyzs{};
		std::vector<double> const parVals{ samplesAccordingTo(parSpec) };
		std::vector<double> const radVals{ samplesAccordingTo(radSpec) };
		xyzs.reserve(parVals.size() * radVals.size());
		for (double const & parVal : parVals)
		{
			XYZ const xyzDir
				{ std::cos(parVal) * std::cos(lonVal)
				, std::cos(parVal) * std::sin(lonVal)
				, std::sin(parVal)
				};
			for (double const & radVal : radVals)
			{
				XYZ const xyz{ radVal * xyzDir };
				xyzs.emplace_back(xyz);
			}
		}
		return xyzs;
	}

	//! Collection of longitude angle values
	std::vector<double>
	bulkSamplesLon
		( std::size_t const numBulk = 8u
		)
	{
		std::vector<double> samps{};
		samps.reserve(numBulk + 3u);
		// include several key values
		peri::sim::SampleSpec const lonSpec{ numBulk, sRangeLon };
		samps.emplace_back(lonSpec.first());
		samps.emplace_back(0.);
		samps.emplace_back(lonSpec.last());
		// fill bulk points
		for (std::size_t nn{0u} ; nn < lonSpec.size() ; ++nn)
		{
			samps.emplace_back(lonSpec.valueAtIndex(nn));
		}
		return samps;
	}

	//! Collection of parallel(latitude) angle values
	std::vector<double>
	bulkSamplesPar
		( std::size_t const numBulk = 8u
		)
	{
		std::vector<double> samps{};
		samps.reserve(numBulk + 5u);
		double const pi{ peri::pi() };
		// be sure to include several key values
		samps.emplace_back(-.5*pi);
		samps.emplace_back(-.25*pi);
		samps.emplace_back(0.);
		samps.emplace_back(.25*pi);
		samps.emplace_back(.5*pi);
		// fill bulk points
		peri::sim::SampleSpec const parSpec{ numBulk, sRangePar };
		for (std::size_t nn{0u} ; nn < numBulk ; ++nn)
		{
			samps.emplace_back(parSpec.valueAtIndex(nn));
		}
		return samps;
	}

	//! Collection of altitude values
	std::vector<double>
	bulkSamplesAlt
		( std::size_t const numBulk = 8u
		)
	{
		std::vector<double> samps{};
		samps.reserve(numBulk + 3u);
		// be sure to include several key values
		samps.emplace_back(-100000.);
		samps.emplace_back(0.);
		samps.emplace_back(100000.);
		// fill bulk points
		double const alt0{ -1.e+5 };
		double const altDelta{ (2.e+5) / static_cast<double>(numBulk) };
		for (std::size_t nn{0u} ; nn < numBulk ; ++nn)
		{
			double const alt{ alt0 + static_cast<double>(nn)*altDelta };
			samps.emplace_back(alt);
		}
		return samps;
	}

	//! Collection of LPA locations (spanning domain of validity as defined)
	std::vector<peri::LPA>
	comboSamplesLpa
		( std::vector<double> const & lonSamps
		, std::vector<double> const & parSamps
		, std::vector<double> const & altSamps
		)
	{
		std::vector<peri::LPA> lpas;
		lpas.reserve(lonSamps.size() * parSamps.size() * altSamps.size());
		for (double const & lonSamp : lonSamps)
		{
			for (double const & parSamp : parSamps)
			{
				for (double const & altSamp : altSamps)
				{
					lpas.emplace_back(peri::LPA{ lonSamp, parSamp, altSamp });
				}
			}
		}
		return lpas;
	}

	//! Collection of LPA locations (spanning domain of validity as defined)
	std::vector<peri::LPA>
	bulkSamplesLpa
		( std::size_t const lonBulk = 8u
		, std::size_t const parBulk = 8u
		, std::size_t const altBulk = 8u
		)
	{
		std::vector<double> const lonSamps{ bulkSamplesLon(lonBulk) };
		std::vector<double> const parSamps{ bulkSamplesPar(parBulk) };
		std::vector<double> const altSamps{ bulkSamplesAlt(altBulk) };
		return comboSamplesLpa(lonSamps, parSamps, altSamps);
	}

} // [peri::sim]

#endif // peri_Sim_INCL_