/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/import/GenWayDat.h>

#include <algorithm>
#include <cassert>

#include <osmscout/DataFile.h>

#include <osmscout/Util.h>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  std::string WayDataGenerator::GetDescription() const
  {
    return "Generate 'ways.dat'";
  }

  bool WayDataGenerator::ReadRestrictionRelations(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const TypeConfig& typeConfig,
                                                  std::map<Id,std::vector<Way::Restriction> >& restrictions)
  {
    FileScanner scanner;
    uint32_t    rawRelCount=0;

    // List of restrictions for a way
    TypeId      restrictionPosId=typeConfig.GetRelationTypeId("restriction_only_straight_on");
    TypeId      restrictionNegId=typeConfig.GetRelationTypeId("restriction_no_straight_on");

    assert(restrictionPosId!=typeIgnore);
    assert(restrictionNegId!=typeIgnore);

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawrels.dat"))) {
      progress.Error("Cannot open 'rawrels.dat'");
      return false;
    }

    if (!scanner.Read(rawRelCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t r=1; r<=rawRelCount; r++) {
      progress.SetProgress(r,rawRelCount);

      RawRelation relation;

      if (!relation.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(rawRelCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (relation.GetType()==restrictionPosId ||
          relation.GetType()==restrictionNegId) {
        Id               from=0;
        Way::Restriction restriction;

        restriction.members.resize(1,0);

        if (relation.GetType()==restrictionPosId) {
          restriction.type=Way::rstrAllowTurn;
        }
        else if (relation.GetType()==restrictionNegId) {
          restriction.type=Way::rstrForbitTurn;
        }
        else {
          continue;
        }

        for (size_t i=0; i<relation.members.size(); i++) {
          if (relation.members[i].type==RawRelation::memberWay &&
              relation.members[i].role=="from") {
            from=relation.members[i].id;
          }
          else if (relation.members[i].type==RawRelation::memberWay &&
                   relation.members[i].role=="to") {
            restriction.members[0]=relation.members[i].id;
          }
          else if (relation.members[i].type==RawRelation::memberNode &&
                   relation.members[i].role=="via") {
            restriction.members.push_back(relation.members[i].id);
          }
        }

        if (from!=0 &&
            restriction.members[1]!=0 &&
            restriction.members.size()>1) {
          restrictions[from].push_back(restriction);
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawrels.dat'");
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(restrictions.size())+" restrictions");

    return true;
  }

  bool WayDataGenerator::ReadWayEndpoints(const ImportParameter& parameter,
                                          Progress& progress,
                                          const TypeConfig& typeConfig,
                                          EndPointWayMap& endPointWayMap)
  {
    FileScanner scanner;
    uint32_t    rawWayCount=0;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                                      true,
                                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=rawWayCount; w++) {
      progress.SetProgress(w,rawWayCount);

      RawWay rawWay;

      if (!rawWay.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(rawWayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (rawWay.GetType()==typeIgnore) {
        continue;
      }

      if (typeConfig.GetTypeInfo(rawWay.GetType()).GetIgnore()) {
        continue;
      }

      if (rawWay.IsArea()) {
        continue;
      }

      endPointWayMap[rawWay.GetNodes().front()].push_back(rawWay.GetId());
      endPointWayMap[rawWay.GetNodes().back()].push_back(rawWay.GetId());
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    return true;
  }

  bool WayDataGenerator::ReadAreasIncludingEndpoints(const ImportParameter& parameter,
                                                     Progress& progress,
                                                     const TypeConfig& typeConfig,
                                                     const EndPointWayMap& endPointWayMap,
                                                     std::set<Id>& endPointAreaSet)
  {
    FileScanner scanner;
    uint32_t    rawWayCount=0;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                                      true,
                                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=rawWayCount; w++) {
      progress.SetProgress(w,rawWayCount);

      RawWay rawWay;

      if (!rawWay.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(rawWayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (rawWay.GetType()==typeIgnore) {
        continue;
      }

      if (typeConfig.GetTypeInfo(rawWay.GetType()).GetIgnore()) {
        continue;
      }

      if (!rawWay.IsArea()) {
        continue;
      }

      for (size_t i=0; i<rawWay.GetNodeCount(); i++) {
        EndPointWayMap::const_iterator nodeUse;

        nodeUse=endPointWayMap.find(rawWay.GetNodeId(i));

        if (nodeUse!=endPointWayMap.end()) {
          endPointAreaSet.insert(rawWay.GetNodeId(i));
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    return true;
  }

  void WayDataGenerator::GetWayMergeCandidates(const RawWay& way,
                                               const EndPointWayMap& endPointWayMap,
                                               const std::set<Id>& wayBlacklist,
                                               std::set<Id>& candidates)
  {
    EndPointWayMap::const_iterator endPoints;

    endPoints=endPointWayMap.find(way.GetNodes().front());

    if (endPoints!=endPointWayMap.end()) {
      for (std::list<Id>::const_iterator id=endPoints->second.begin();
          id!=endPoints->second.end();
          id++) {
        if (*id>way.GetId() &&
            wayBlacklist.find(*id)==wayBlacklist.end()) {
          candidates.insert(*id);
        }
      }
    }

    endPoints=endPointWayMap.find(way.GetNodes().back());

    if (endPoints!=endPointWayMap.end()) {
      for (std::list<Id>::const_iterator id=endPoints->second.begin();
          id!=endPoints->second.end();
          id++) {
        if (*id>way.GetId() &&
            wayBlacklist.find(*id)==wayBlacklist.end()) {
          candidates.insert(*id);
        }
      }
    }
  }

  bool WayDataGenerator::LoadWays(Progress& progress,
                                  FileScanner& scanner,
                                  NumericIndex<Id,RawWay>& rawWayIndex,
                                  const std::set<Id>& ids,
                                  std::map<Id,RawWayRef>& ways)
  {
    std::vector<FileOffset> offsets;

    if (!rawWayIndex.GetOffsets(ids,offsets)) {
      return false;
    }

    FileOffset oldPos;

    if (!scanner.GetPos(oldPos)){
      progress.Error("Error while getting current file position");
      return false;
    }

    for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
         offset!=offsets.end();
         offset++) {
      if (!scanner.SetPos(*offset)) {
        progress.Error("Error while moving to way at offset " + NumberToString(*offset));
        return false;
      }

      RawWayRef way=new RawWay();

      way->Read(scanner);

      if (scanner.HasError()) {
        progress.Error("Error while loading way at offset " + NumberToString(*offset));
        return false;
      }

      ways[way->GetId()]=way;
    }

    if (!scanner.SetPos(oldPos)) {
      progress.Error("Error while resetting current file position");
      return false;
    }

    return true;
  }

  bool WayDataGenerator::CompareWays(const RawWay& a,
                                     const RawWay& b) const
  {
    if (a.GetType()!=b.GetType()) {
      return false;
    }

    if (a.GetTags().size()!=b.GetTags().size()) {
      return false;
    }

    for (size_t t=0; t<b.GetTags().size(); t++) {
      if (a.GetTags()[t].key!=b.GetTags()[t].key) {
        return false;
      }

      if (a.GetTags()[t].value!=b.GetTags()[t].value) {
        return false;
      }
    }

    return true;
  }

  bool WayDataGenerator::JoinWays(Progress& progress,
                                  const TypeConfig& typeConfig,
                                  FileScanner& scanner,
                                  std::vector<RawWay>& rawWays,
                                  size_t blockCount,
                                  EndPointWayMap& endPointWayMap,
                                  NumericIndex<Id,RawWay>& rawWayIndex,
                                  std::set<Id> & wayBlacklist,
                                  size_t& mergeCount)
  {
    std::vector<bool> hasBeenMerged(blockCount, true);
    bool              somethingHasMerged=true;
    SilentProgress    silentProgress;

    while (somethingHasMerged) {
      somethingHasMerged=false;

      // Collect all candidate ids for all ways, that still have potential merges
      // to be verified

      std::set<Id> allCandidates;

      for (size_t b=0; b<blockCount; b++) {
        if (hasBeenMerged[b] &&
            !rawWays[b].IsArea()) {
          RawWay& rawWay=rawWays[b];

          GetWayMergeCandidates(rawWay,
                                endPointWayMap,
                                wayBlacklist,
                                allCandidates);
        }
      }


      if (allCandidates.empty()) {
        continue;
      }

      // Now load all candidate ways by id as calculated above

      std::map<Id,RawWayRef> ways;

      progress.Info("Loading " + NumberToString(allCandidates.size())+ " candidate(s)");
      if (!LoadWays(progress,
                    scanner,
                    rawWayIndex,
                    allCandidates,
                    ways)) {
        return false;
      }

      allCandidates.clear();

      progress.Info("Merging ways");
      for (size_t b=0; b<blockCount; b++) {
        if (hasBeenMerged[b] &&
            !rawWays[b].IsArea()) {
          RawWay& rawWay=rawWays[b];

          hasBeenMerged[b]=false;

          // The way has already been merged (within this block)
          if (wayBlacklist.find(rawWay.GetId())!=wayBlacklist.end()) {
            continue;
          }

          SegmentAttributes origAttributes;
          std::vector<Tag>  origTags(rawWay.GetTags());
          bool              origReverseNodes;

          origAttributes.type=rawWay.GetType();
          if (!origAttributes.SetTags(silentProgress,
                                      typeConfig,
                                      rawWay.GetId(),
                                      rawWay.IsArea(),
                                      origTags,
                                      origReverseNodes)) {
            continue;
          }

          std::set<Id> candidates;

          GetWayMergeCandidates(rawWay,
                                endPointWayMap,
                                wayBlacklist,
                                candidates);

          for (std::set<Id>::const_iterator id=candidates.begin();
              id!=candidates.end();
              ++id) {
            std::map<Id,RawWayRef>::const_iterator wayEntry;

            wayEntry=ways.find(*id);

            if (wayEntry==ways.end()) {
              progress.Error("Error while loading merge candidate "+
                             NumberToString(*id) +
                             " (Internal error?)");
              continue;
            }

            RawWayRef candidate=wayEntry->second;

            if (candidate->IsArea()) {
              continue;
            }

            // We do not merge against ways, that are already on the blacklist
            // because of previous merges
            if(wayBlacklist.find(*id)!=wayBlacklist.end()) {
              continue;
            }

            SegmentAttributes matchAttributes;
            std::vector<Tag>  matchTags(candidate->GetTags());
            bool              matchReverseNodes;

            matchAttributes.type=candidate->GetType();
            if (!matchAttributes.SetTags(silentProgress,
                                         typeConfig,
                                         candidate->GetId(),
                                         candidate->IsArea(),
                                         matchTags,
                                         matchReverseNodes)) {
              continue;
            }

            if (origAttributes!=matchAttributes) {
              continue;
            }

            if (origReverseNodes!=matchReverseNodes) {
              continue;
            }

            if (rawWay.GetId()==56433551 || candidate->GetId()==56433551) {
              std::cout << "Merging " << rawWay.GetId() << " with " << candidate->GetId() << std::endl;
            }

            hasBeenMerged[b]=true;
            somethingHasMerged=true;

            std::vector<Id> origNodes(rawWay.GetNodes());

            if (origReverseNodes) {
              std::reverse(origNodes.begin(),origNodes.end());
              origReverseNodes=false;
            }

            if (matchReverseNodes) {
              std::vector<Id> matchNodes(candidate->GetNodes());

              std::reverse(matchNodes.begin(),matchNodes.end());
              candidate->SetNodes(matchNodes);
              matchReverseNodes=false;
            }

            if (!origAttributes.IsOneway() &&
                origNodes.front()==candidate->GetNodes().front()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.insert(origNodes.begin(),candidate->GetNodeId(i));
              }
            }
            else if (origNodes.front()==candidate->GetNodes().back()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.insert(origNodes.begin(),candidate->GetNodeId(candidate->GetNodeCount()-1-i));
              }
            }
            else if (origNodes.back()==candidate->GetNodes().front()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.push_back(candidate->GetNodeId(i));
              }
            }
            else if (!origAttributes.IsOneway() &&
                     origNodes.back()==candidate->GetNodes().back()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.push_back(candidate->GetNodeId(candidate->GetNodeCount()-1-i));
              }
            }
            else {
              assert(true);
            }

            wayBlacklist.insert(candidate->GetId());

            rawWay.SetNodes(origNodes);

            mergeCount++;

            if (rawWay.GetId()==56433551 || candidate->GetId()==56433551) {
              for (size_t n=0; n<rawWay.GetNodeCount(); n++) {
                std::cout << rawWay.GetNodeId(n) << " ";
              }
              std::cout << std::endl;
            }

            break;
          }
        }
      }
    }

    return true;
  }

  bool WayDataGenerator::Import(const ImportParameter& parameter,
                                Progress& progress,
                                const TypeConfig& typeConfig)
  {
    progress.SetAction("Generate ways.dat");

    FileScanner                                 scanner;
    FileWriter                                  writer;
    uint32_t                                    rawWayCount=0;

    // List of restrictions for a way
    std::map<Id,std::vector<Way::Restriction> > restrictions;

    uint32_t                                    writtenWayCount=0;

    EndPointWayMap                              endPointWayMap;
    std::set<Id>                                endPointAreaSet;

    std::set<Id>                                wayBlacklist;

    //
    // handling of restriction relations
    //

    progress.SetAction("Scanning for restriction relations");

    if (!ReadRestrictionRelations(parameter,
                                  progress,
                                  typeConfig,
                                  restrictions)) {
      return false;
    }

    //
    // Building a map of way endpoint ids and list of ways having this point as endpoint
    //

    progress.SetAction("Collecting way endpoints");

    if (!ReadWayEndpoints(parameter,
                          progress,
                          typeConfig,
                          endPointWayMap)) {
      return false;
    }

    //
    // Now enriching this map with areas that include this endpoints as node
    //

    progress.SetAction("Enriching way endpoints");

    if (!ReadAreasIncludingEndpoints(parameter,
                                     progress,
                                     typeConfig,
                                     endPointWayMap,
                                     endPointAreaSet)) {
      return false;
    }

    progress.Info(NumberToString(endPointWayMap.size())+ " endpoints collected");

    DataFile<RawNode>       nodeDataFile("rawnodes.dat",
                                         "rawnode.idx",
                                         parameter.GetRawNodeDataCacheSize(),
                                         parameter.GetRawNodeIndexCacheSize());
    NumericIndex<Id,RawWay> rawWayIndex("rawway.idx",parameter.GetRawWayIndexCacheSize());

    if (!nodeDataFile.Open(parameter.GetDestinationDirectory(),
                           parameter.GetRawNodeIndexMemoryMaped(),
                           parameter.GetRawNodeDataMemoryMaped())) {
      std::cerr << "Cannot open raw node data file!" << std::endl;
      return false;
    }

    if (!rawWayIndex.Open(parameter.GetDestinationDirectory(),
                           parameter.GetRawWayIndexMemoryMaped())) {
      std::cerr << "Cannot open raw way index file!" << std::endl;
      return false;
    }

    //
    // Writing ways
    //

    progress.SetAction("Writing ways");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                                      true,
                                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "ways.dat"))) {
      progress.Error("Cannot create 'ways.dat'");
      return false;
    }

    writer.Write(writtenWayCount);

    uint32_t            currentWay=1;
    size_t              mergeCount=0;
    std::vector<RawWay> block(parameter.GetRawWayBlockSize());

    while (currentWay<rawWayCount) {
      size_t blockCount=0;

      progress.SetAction("Loading up to " + NumberToString(block.size()) + " ways");
      while (blockCount<block.size() && currentWay<rawWayCount) {
        progress.SetProgress(currentWay,rawWayCount);

        if (!block[blockCount].Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(currentWay)+" of "+
                         NumberToString(rawWayCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        currentWay++;

        std::set<Id>::iterator blacklistEntry=wayBlacklist.find(block[blockCount].GetId());

        if (blacklistEntry!=wayBlacklist.end()) {
          wayBlacklist.erase(blacklistEntry);

          continue;
        }

        if (block[blockCount].GetType()==typeIgnore) {
          continue;
        }

        if (typeConfig.GetTypeInfo(block[blockCount].GetType()).GetIgnore()) {
          continue;
        }

        if (block[blockCount].GetNodeCount()<2) {
          progress.Error(std::string("Way ")+
                         NumberToString(block[blockCount].GetId())+" has only "+
                         NumberToString(block[blockCount].GetNodeCount())+
                         " node(s) but requires at least 2 nodes");
          continue;
        }


        blockCount++;
      }

      std::set<Id>            nodeIds;
      std::vector<RawNodeRef> nodes;
      std::map<Id,RawNodeRef> nodesMap;

      progress.SetAction("Merging ways");
      // Join with potential joined ways
      if (!JoinWays(progress,
                    typeConfig,
                    scanner,
                    block,
                    blockCount,
                    endPointWayMap,
                    rawWayIndex,
                    wayBlacklist,
                    mergeCount)) {
        return false;
      }

      progress.SetAction("Writing ways");

      for (size_t w=0; w<blockCount; w++) {
        for (size_t n=0; n<block[w].GetNodeCount(); n++) {
          nodeIds.insert(block[w].GetNodeId(n));
        }
      }

      if (!nodeDataFile.Get(nodeIds,nodes)) {
        std::cerr << "Cannot read nodes!" << std::endl;
        continue;
      }

      for (std::vector<RawNodeRef>::const_iterator node=nodes.begin();
          node!=nodes.end();
          node++) {
        nodesMap[(*node)->GetId()]=*node;
      }

      nodes.clear();

      for (size_t w=0; w<blockCount; w++) {
        // Way has been joined, no need to write it
        if (wayBlacklist.find(block[w].GetId())!=wayBlacklist.end()) {
          continue;
        }

        std::vector<Tag> tags(block[w].GetTags());
        Way              way;
        bool             reverseNodes=false;

        way.SetId(block[w].GetId());
        way.SetType(block[w].GetType());


        if (!way.SetTags(progress,
                         typeConfig,
                         block[w].IsArea(),
                         tags,
                         reverseNodes)) {
          continue;
        }

        way.nodes.resize(block[w].GetNodeCount());

        bool success=true;
        for (size_t n=0; n<block[w].GetNodeCount(); n++) {
          std::map<Id,RawNodeRef>::const_iterator node=nodesMap.find(block[w].GetNodeId(n));

          if (node==nodesMap.end()) {
            progress.Error("Cannot resolve node with id "+
                           NumberToString(block[w].GetNodeId(n))+
                           " for Way "+
                           NumberToString(way.GetId()));
            success=false;
            break;
          }

          way.nodes[n].SetId(node->second->GetId());
          way.nodes[n].SetCoordinates(node->second->GetLat(),node->second->GetLon());
        }

        if (!success) {
          continue;
        }

        if (reverseNodes) {
          std::reverse(way.nodes.begin(),way.nodes.end());
        }

        // startIsJoint/endIsJoint

        if (!way.IsArea()) {
          EndPointWayMap::const_iterator wayJoint;
          std::set<Id>::const_iterator   areaJoint;

          wayJoint=endPointWayMap.find(way.nodes.front().GetId());

          if (wayJoint!=endPointWayMap.end() &&
              wayJoint->second.size()<2) {
            wayJoint=endPointWayMap.end();
          }

          if (wayJoint==endPointWayMap.end()) {
            areaJoint=endPointAreaSet.find(way.nodes.front().GetId());
          }

          way.SetStartIsJoint(wayJoint!=endPointWayMap.end() || areaJoint!=endPointAreaSet.end());

          wayJoint=endPointWayMap.find(way.nodes.back().GetId());

          if (wayJoint!=endPointWayMap.end() && wayJoint->second.size()<2) {
            wayJoint=endPointWayMap.end();
          }

          if (wayJoint==endPointWayMap.end()) {
            areaJoint=endPointAreaSet.find(way.nodes.back().GetId());
          }

          way.SetEndIsJoint(wayJoint!=endPointWayMap.end() || areaJoint!=endPointAreaSet.end());
        }

        // Restrictions

        std::map<Id,std::vector<Way::Restriction> >::iterator iter=restrictions.find(way.GetId());

        if (iter!=restrictions.end()) {
          way.SetRestrictions(iter->second);
        }

        way.Write(writer);
        writtenWayCount++;
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    writer.SetPos(0);
    writer.Write(writtenWayCount);


    if (!writer.Close()) {
      return false;
    }

    if (!rawWayIndex.Close()) {
      return false;
    }

    if (!nodeDataFile.Close()) {
      return false;
    }

    progress.Info(NumberToString(rawWayCount) + " raw way(s) read, "+
                  NumberToString(writtenWayCount) + " way(s) written, "+
                  NumberToString(mergeCount) + " merges");

    // Cleaning up...

    endPointWayMap.clear();
    endPointAreaSet.clear();
    restrictions.clear();
    wayBlacklist.clear();

    return true;
  }
}

