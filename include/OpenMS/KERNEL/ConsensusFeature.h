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
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// --------------------------------------------------------------------------
// $Maintainer: Eva Lange $
// --------------------------------------------------------------------------

#ifndef OPENMS_KERNEL_CONSENSUSFEATURE_H
#define OPENMS_KERNEL_CONSENSUSFEATURE_H

#include <OpenMS/KERNEL/FeatureMap.h>
#include <OpenMS/DATASTRUCTURES/DRange.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/Group.h>

namespace OpenMS
{

  /**
    @brief A 2-dimensional consensus feature.
    
    A consensus feature represents corresponding features in multiple featuremaps.
    
    @ingroup Kernel
  */
  template < typename ContainerT = FeatureMap< > >
  class ConsensusFeature :  	
    public Feature,
    public Group< ContainerT >
  {
    public:
      /**
        @name Type definitions
      */
      //@{
      typedef Feature BaseElementType;
      typedef ContainerT ElementContainerType;
      typedef typename ElementContainerType::value_type ElementType;
      typedef Group< ElementContainerType > Group;

      typedef DPosition < 2> PositionType;
      typedef DoubleReal IntensityType;
      typedef DRange<2> PositionBoundingBoxType;
      typedef DRange<1> IntensityBoundingBoxType;
      //@}


      /** @name Constructors and Destructor
      */
      //@{
      /// Default constructor
      ConsensusFeature()
          : BaseElementType(),
          Group(),
          position_range_(),
          intensity_range_()
      {}

      ///
      ConsensusFeature(const PositionType& pos, IntensityType i)
          : BaseElementType(),
          Group(),
          position_range_(),
          intensity_range_()
      {
        this->getPosition() = pos;
        this->setIntensity(i);
      }

      /// Constructor for a singleton consensus feature
      ConsensusFeature(UInt map_index,  UInt feature_index, const ElementType& feature)
    {
        IndexTuple i(map_index,feature_index,feature.getIntensity(),feature.getPosition());
        i.setTransformedPosition(feature.getPosition());
        this->insert(i);

        this->getPosition() = feature.getPosition();
        this->setIntensity(feature.getIntensity());
        
        position_range_.setMinMax(feature.getPosition(),feature.getPosition());
        intensity_range_.setMinMax(feature.getIntensity(),feature.getIntensity());
      }

      /// Constructor
      ConsensusFeature(UInt map_1_index, UInt feature_index_1, const ElementType& feature_1,
                       UInt map_2_index, UInt feature_index_2, const ElementType& feature_2)
      {
        IndexTuple i1(map_1_index,feature_index_1, feature_1.getIntensity(),feature_1.getPosition());
        i1.setTransformedPosition(feature_1.getPosition());
        this->insert(i1,false);
        IndexTuple i2(map_2_index,feature_index_2, feature_2.getIntensity(),feature_2.getPosition());
        i2.setTransformedPosition(feature_2.getPosition());
        this->insert(i2);

        computeConsensus_();
      }

      /// Constructor
      ConsensusFeature(UInt map_index, UInt feature_index, const ElementType& feature, const ConsensusFeature& c_feature)
      {
        Group::operator=(c_feature);
        IndexTuple i(map_index,feature_index,feature.getIntensity(),feature.getPosition());
        i.setTransformedPosition(feature.getPosition());
        this->insert(i);

        computeConsensus_();
      }

      /// Constructor
      ConsensusFeature(const ConsensusFeature& c_feature_1, const ConsensusFeature& c_feature_2)
      {
        Group::operator=(c_feature_1);

        for (typename Group::iterator it = c_feature_2.begin(); it != c_feature_2.end(); ++it)
        {
          this->insert(*it);
        }

        computeConsensus_();
      }


      /// Copy constructor
      inline ConsensusFeature(const ConsensusFeature& source)
          : BaseElementType(source),
          Group(source),
          position_range_(source.position_range_),
          intensity_range_(source.intensity_range_)
      {
      }

      /// Assignement operator
      ConsensusFeature& operator=(const ConsensusFeature& source)
      {
        if (&source==this)
          return *this;

        Group::operator=(source);
        BaseElementType::operator=(source);
        position_range_=source.position_range_;
        intensity_range_=source.intensity_range_;

        return *this;
      }

      /// Destructor
      virtual ~ConsensusFeature()
   		{
   		}
      //@}


      void insert(const IndexTuple& tuple, bool recalculate = true)
      {
        Group::insert(tuple);

        if (recalculate) computeConsensus_();
      }

      /// Non-mutable access to the position range
      inline const PositionBoundingBoxType& getPositionRange() const
      {
        return position_range_;
      }
      /// Mutable access to the position range
      inline PositionBoundingBoxType& getPositionRange()
      {
        return position_range_;
      }
      /// Set the position range
      inline void setPositionRange(const PositionBoundingBoxType& p)
      {
        position_range_= p;
      }

      /// Non-mutable access to the intensity range
      inline const IntensityBoundingBoxType& getIntensityRange() const
      {
        return intensity_range_;
      }
      /// Mutable access to the intensity range
      inline IntensityBoundingBoxType& getIntensityRange()
      {
        return intensity_range_;
      }
      /// Set the intensity range
      inline void setIntensityRange(const IntensityBoundingBoxType& i)
      {
        intensity_range_ = i;
      }

      /// Non-mutable access to the combined features
      inline const Group& getFeatures() const
      {
        return *this;
      }
      /// Mutable access to the combined features
      inline Group& getFeatures()
      {
        return *this;
      }
      /// Set the combined features
      inline void setFeatures(const Group& g)
      {
        Group::operator=(g);
      }


    protected:
      PositionBoundingBoxType position_range_;
      IntensityBoundingBoxType intensity_range_;

      // compute the consensus attributes like intensity and position as well as the position and intensity range given by the group elements
      void computeConsensus_()
      {
        unsigned int n = Group::size();
        DPosition<2> sum_position;
        DPosition<2> pos_min(std::numeric_limits<DoubleReal>::max());
        DPosition<2> pos_max(std::numeric_limits<DoubleReal>::min());
        DPosition<1> sum_intensities = 0;
        DPosition<1> int_min(std::numeric_limits<DoubleReal>::max());
        DPosition<1> int_max(std::numeric_limits<DoubleReal>::min());
        for (typename Group::const_iterator it = Group::begin(); it != Group::end(); ++it)
        {
          DPosition<1> act_int = it->getIntensity();
          DPosition<2> act_pos = it->getTransformedPosition();

          if (int_min > act_int)
          {
            int_min = act_int;
          }
          if (int_max < act_int)
          {
            int_max = act_int;
          }

          for (UInt dim=0; dim < 2; ++dim)
          {
            if (act_pos[dim] > pos_max[dim])
              pos_max[dim] = act_pos[dim];
            if (act_pos[dim] < pos_min[dim])
              pos_min[dim] = act_pos[dim];
          }

          sum_intensities += act_int;
          sum_position += act_pos;
        }

        for (UInt dim = 0; dim< 2 ; ++dim)
        {
          this->position_[dim] = sum_position[dim] / n;
        }
        this->intensity_ = sum_intensities[0] / n;

        intensity_range_.setMinMax(int_min,int_max);
        position_range_.setMinMax(pos_min,pos_max);
      }
  };

  ///Print the contents to a stream.
  template < typename ContainerT >
  std::ostream& operator << (std::ostream& os, const ConsensusFeature<ContainerT>& cons)
  {
    os << "---------- CONSENSUS ELEMENT BEGIN -----------------\n";
    os << "Position: " << cons.getPosition()<< std::endl;
    os << "Intensity " << cons.getIntensity() << std::endl;
    os << "Position range " << cons.getPositionRange() << std::endl;
    os << "Intensity range " << cons.getIntensityRange() << std::endl;
    os << "Grouped elements: " << std::endl;

    unsigned int i = 1;
    os << "Size " << cons.count() << std::endl;
    for (typename ConsensusFeature<ContainerT>::Group::const_iterator it = cons.begin(); it != cons.end(); ++it, ++i)
    {
      os << "Element: " << i << std::endl;
      os << "Map index: " << it->getMapIndex() << " feature index " << it->getElementIndex() << std::endl;
      os <<  "Transformed Position: " << it->getTransformedPosition() << std::endl;
      os <<  "Intensity: " << it->getIntensity() << std::endl;
    }
    os << "---------- CONSENSUS ELEMENT END ----------------- " << std::endl;

    return os;
  }

} // namespace OpenMS

#endif // OPENMS_KERNEL_DFEATURE_H
