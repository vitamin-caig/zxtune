/**
 *
 * @file
 *
 * @brief  Xsf-based files structure support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/xsf/xsf_factory.h"
#include "module/players/xsf/xsf_file.h"
#include "module/players/xsf/xsf_metainformation.h"

#include "formats/chiptune/container.h"

#include "string_view.h"

namespace Module::XSF
{
  Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, File& file);
  Formats::Chiptune::Container::Ptr Parse(StringView name, const Binary::Container& data, File& file);

  // Generalizes the per-format leaf behavior of multi-file loading: which sections make
  // up a file of the particular format and whether a missing section is tolerated.
  // The sections are passed unconditionally, so the particular format decides the actual
  // presence requirements.
  class MergeTarget
  {
  public:
    virtual ~MergeTarget() = default;
    virtual void AddProgramSection(Binary::Container::Ptr program) = 0;
    virtual void AddReservedSection(Binary::Container::Ptr reserved) = 0;
    virtual void AddMeta(const MetaInformation& meta) = 0;
  };

  // Dependency traversal order differs between the formats, see specs cited in the implementations
  void MergeProgramSectionsMultiLibrary(const File& data, const FilesMap& additionalFiles, MergeTarget& dst,
                                        uint_t level = 1);
  void MergeSectionsSingleLibrary(const File& data, const FilesMap& additionalFiles, MergeTarget& dst,
                                  uint_t level = 1);
  void MergeReservedSectionsDepsFirst(const File& data, const FilesMap& additionalFiles, MergeTarget& dst,
                                      uint_t level = 1);
  void MergeMeta(const File& data, const FilesMap& additionalFiles, MergeTarget& dst, uint_t level = 1);
}  // namespace Module::XSF
