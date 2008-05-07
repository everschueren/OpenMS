// -*- Mode: C++; tab-width: 2; -*-
// vi: set ts=2:
//
// --------------------------------------------------------------------------
//                   OpenMS Mass Spectrometry Framework
// --------------------------------------------------------------------------
//  Copyright (C) 2003-2008 -- Oliver Kohlbacher, Knut Reinert
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// --------------------------------------------------------------------------
// $Maintainer: Clemens Groepl, Marc Sturm $
// --------------------------------------------------------------------------

#ifndef OPENMS_ANALYSIS_MAPMATCHING_FEATUREGROUPINGALGORITHMLABELED_H
#define OPENMS_ANALYSIS_MAPMATCHING_FEATUREGROUPINGALGORITHMLABELED_H

#include <OpenMS/ANALYSIS/MAPMATCHING/FeatureGroupingAlgorithm.h>

namespace OpenMS
{
	/**
		@brief A map feature grouping algorithm for labeling techniques with two labels.
		
		It takes one maps and searches for corresponding features with a defined distance in RT and m/z.
	*/
	class FeatureGroupingAlgorithmLabeled
	 : public FeatureGroupingAlgorithm
	{
		public:
			/// Default constructor
			FeatureGroupingAlgorithmLabeled();

			/// Destructor
			virtual ~FeatureGroupingAlgorithmLabeled();
			
			//Docu in base class
			virtual void group(const std::vector< FeatureMap<> >& maps, ConsensusMap<>& out);

			///Creates a new instance of this class (for Factory)
			static FeatureGroupingAlgorithm* create()
			{
				return new FeatureGroupingAlgorithmLabeled();
			}
			
			///Returns the product name (for the Factory)
			static String getProductName()
			{
				return "labeled";
			}
			
		private:

			///Copy constructor is not implemented -> private
			FeatureGroupingAlgorithmLabeled(const FeatureGroupingAlgorithmLabeled& );
			///Assignment operator is not implemented -> private
			FeatureGroupingAlgorithmLabeled& operator=(const FeatureGroupingAlgorithmLabeled& );
			
	};

} // namespace OpenMS

#endif // OPENMS_ANALYSIS_MAPMATCHING_FEATUREGROUPINGALGORITHMLABELED_H
