/* Generated by /home/asymingt/Workspace/thirdparty/linuxptp/thirdparty/OpenDDS/bin/opendds_idl version 3.7 (ACE version 6.2a_p7) running on input file /home/asymingt/Workspace/thirdparty/linuxptp/src/messages/Timeline.idl */
#include "TimelineTypeSupportImpl.h"

#include <cstring>
#include <stdexcept>
#include "dds/DCPS/FilterEvaluator.h"
#include "dds/DCPS/PoolAllocator.h"


/* Begin MODULE: CORBA */


/* End MODULE: CORBA */


/* Begin STRUCT: Timeline */

namespace OpenDDS { namespace DCPS {

void gen_find_size(const Timeline& stru, size_t& size, size_t& padding)
{
  ACE_UNUSED_ARG(stru);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(padding);
  find_size_ulong(size, padding);
  size += ACE_OS::strlen(stru.uuid.in()) + 1;
  if ((size + padding) % 8) {
    padding += 8 - ((size + padding) % 8);
  }
  size += gen_max_marshaled_size(stru.resolution);
  if ((size + padding) % 8) {
    padding += 8 - ((size + padding) % 8);
  }
  size += gen_max_marshaled_size(stru.accuracy);
}

bool operator<<(Serializer& strm, const Timeline& stru)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(stru);
  return (strm << stru.uuid.in())
    && (strm << stru.resolution)
    && (strm << stru.accuracy);
}

bool operator>>(Serializer& strm, Timeline& stru)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(stru);
  return (strm >> stru.uuid.out())
    && (strm >> stru.resolution)
    && (strm >> stru.accuracy);
}

}  }

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
namespace OpenDDS { namespace DCPS {

template<>
struct MetaStructImpl<Timeline> : MetaStruct {
  typedef Timeline T;

  void* allocate() const { return new T; }

  void deallocate(void* stru) const { delete static_cast<T*>(stru); }

  size_t numDcpsKeys() const { return 0; }

  Value getValue(const void* stru, const char* field) const
  {
    const Timeline& typed = *static_cast<const Timeline*>(stru);
    if (std::strcmp(field, "uuid") == 0) {
      return typed.uuid.in();
    }
    if (std::strcmp(field, "resolution") == 0) {
      return typed.resolution;
    }
    if (std::strcmp(field, "accuracy") == 0) {
      return typed.accuracy;
    }
    ACE_UNUSED_ARG(typed);
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct Timeline)");
  }

  Value getValue(Serializer& ser, const char* field) const
  {
    if (std::strcmp(field, "uuid") == 0) {
      TAO::String_Manager val;
      if (!(ser >> val.out())) {
        throw std::runtime_error("Field 'uuid' could not be deserialized");
      }
      return val;
    } else {
      ACE_CDR::ULong len;
      if (!(ser >> len)) {
        throw std::runtime_error("String 'uuid' length could not be deserialized");
      }
      ser.skip(len);
    }
    if (std::strcmp(field, "resolution") == 0) {
      ACE_CDR::Double val;
      if (!(ser >> val)) {
        throw std::runtime_error("Field 'resolution' could not be deserialized");
      }
      return val;
    } else {
      ser.skip(1, 8);
    }
    if (std::strcmp(field, "accuracy") == 0) {
      ACE_CDR::Double val;
      if (!(ser >> val)) {
        throw std::runtime_error("Field 'accuracy' could not be deserialized");
      }
      return val;
    } else {
      ser.skip(1, 8);
    }
    if (!field[0]) {
      return 0;
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not valid for struct Timeline");
  }

  ComparatorBase::Ptr create_qc_comparator(const char* field, ComparatorBase::Ptr next) const
  {
    ACE_UNUSED_ARG(next);
    if (std::strcmp(field, "uuid") == 0) {
      return make_field_cmp(&T::uuid, next);
    }
    if (std::strcmp(field, "resolution") == 0) {
      return make_field_cmp(&T::resolution, next);
    }
    if (std::strcmp(field, "accuracy") == 0) {
      return make_field_cmp(&T::accuracy, next);
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct Timeline)");
  }

  const char** getFieldNames() const
  {
    static const char* names[] = {"uuid", "resolution", "accuracy", 0};
    return names;
  }

  const void* getRawField(const void* stru, const char* field) const
  {
    if (std::strcmp(field, "uuid") == 0) {
      return &static_cast<const T*>(stru)->uuid;
    }
    if (std::strcmp(field, "resolution") == 0) {
      return &static_cast<const T*>(stru)->resolution;
    }
    if (std::strcmp(field, "accuracy") == 0) {
      return &static_cast<const T*>(stru)->accuracy;
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct Timeline)");
  }

  void assign(void* lhs, const char* field, const void* rhs,
    const char* rhsFieldSpec, const MetaStruct& rhsMeta) const
  {
    ACE_UNUSED_ARG(lhs);
    ACE_UNUSED_ARG(field);
    ACE_UNUSED_ARG(rhs);
    ACE_UNUSED_ARG(rhsFieldSpec);
    ACE_UNUSED_ARG(rhsMeta);
    if (std::strcmp(field, "uuid") == 0) {
      static_cast<T*>(lhs)->uuid = *static_cast<const TAO::String_Manager*>(rhsMeta.getRawField(rhs, rhsFieldSpec));
      return;
    }
    if (std::strcmp(field, "resolution") == 0) {
      static_cast<T*>(lhs)->resolution = *static_cast<const CORBA::Double*>(rhsMeta.getRawField(rhs, rhsFieldSpec));
      return;
    }
    if (std::strcmp(field, "accuracy") == 0) {
      static_cast<T*>(lhs)->accuracy = *static_cast<const CORBA::Double*>(rhsMeta.getRawField(rhs, rhsFieldSpec));
      return;
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct Timeline)");
  }

  bool compare(const void* lhs, const void* rhs, const char* field) const
  {
    ACE_UNUSED_ARG(lhs);
    ACE_UNUSED_ARG(field);
    ACE_UNUSED_ARG(rhs);
    if (std::strcmp(field, "uuid") == 0) {
      return 0 == ACE_OS::strcmp(static_cast<const T*>(lhs)->uuid.in(), static_cast<const T*>(rhs)->uuid.in());
    }
    if (std::strcmp(field, "resolution") == 0) {
      return static_cast<const T*>(lhs)->resolution == static_cast<const T*>(rhs)->resolution;
    }
    if (std::strcmp(field, "accuracy") == 0) {
      return static_cast<const T*>(lhs)->accuracy == static_cast<const T*>(rhs)->accuracy;
    }
    throw std::runtime_error("Field " + OPENDDS_STRING(field) + " not found or its type is not supported (in struct Timeline)");
  }
};

template<>
const MetaStruct& getMetaStruct<Timeline>()
{
  static MetaStructImpl<Timeline> msi;
  return msi;
}

void gen_skip_over(Serializer& ser, Timeline*)
{
  ACE_UNUSED_ARG(ser);
  MetaStructImpl<Timeline>().getValue(ser, "");
}

}  }

#endif

/* End STRUCT: Timeline */
