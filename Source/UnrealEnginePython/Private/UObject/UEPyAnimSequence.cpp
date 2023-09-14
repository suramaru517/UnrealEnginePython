#include "UEPyAnimSequence.h"
#include "AnimationUtils.h"
#include "Misc/EngineVersionComparison.h"


PyObject *py_ue_anim_get_skeleton(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	UAnimationAsset *anim = ue_py_check_type<UAnimationAsset>(self);
	if (!anim)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimationAsset.");

	USkeleton *skeleton = anim->GetSkeleton();
	if (!skeleton)
	{
		Py_RETURN_NONE;
	}

	Py_RETURN_UOBJECT((UObject *)skeleton);
}

PyObject *py_ue_anim_get_bone_transform(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	int track_index;
	double frame_time;
	PyObject *py_b_use_raw_data = nullptr;

	if (!PyArg_ParseTuple(args, "if|O:get_bone_transform", &track_index, &frame_time, &py_b_use_raw_data))
	{
		return nullptr;
	}

	UAnimSequence *anim = ue_py_check_type<UAnimSequence>(self);
	if (!anim)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");

	bool bUseRawData = false;
	if (py_b_use_raw_data && PyObject_IsTrue(py_b_use_raw_data))
		bUseRawData = true;

	FTransform OutAtom;
#if UE_VERSION_OLDER_THAN(5, 2, 0)
	anim->GetBoneTransform(OutAtom, track_index, frame_time, bUseRawData);
#else
	anim->GetBoneTransform(OutAtom, FSkeletonPoseBoneIndex(track_index), frame_time, bUseRawData);
#endif

	return py_ue_new_ftransform(OutAtom);
}

PyObject *py_ue_anim_extract_bone_transform(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	PyObject *py_sources;
	double frame_time;

	if (!PyArg_ParseTuple(args, "Of:extract_bone_transform", &py_sources, &frame_time))
	{
		return nullptr;
	}

	UAnimSequence *anim = ue_py_check_type<UAnimSequence>(self);
	if (!anim)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");

	ue_PyFRawAnimSequenceTrack *rast = py_ue_is_fraw_anim_sequence_track(py_sources);
	if (rast)
	{
		FTransform OutAtom;
#if UE_VERSION_OLDER_THAN(5, 2, 0)
		FAnimationUtils::ExtractTransformFromTrack(frame_time, anim->GetNumberOfSampledKeys(), anim->GetPlayLength(), rast->raw_anim_sequence_track, anim->Interpolation, OutAtom);
#else
		FAnimationUtils::ExtractTransformFromTrack(rast->raw_anim_sequence_track, frame_time, anim->GetNumberOfSampledKeys(), anim->GetPlayLength(), anim->Interpolation, OutAtom);
#endif

		return py_ue_new_ftransform(OutAtom);
	}

	return PyErr_Format(PyExc_Exception, "argument is not an FRawAnimSequenceTrack");

}

PyObject *py_ue_anim_extract_root_motion(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	float start_time;
	float delta_time;
	PyObject *py_b_allow_looping = nullptr;

	if (!PyArg_ParseTuple(args, "ff|O:extract_root_motion", &start_time, &delta_time, &py_b_allow_looping))
	{
		return nullptr;
	}

	UAnimSequence *anim = ue_py_check_type<UAnimSequence>(self);
	if (!anim)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");

	bool bAllowLooping = false;
	if (py_b_allow_looping && PyObject_IsTrue(py_b_allow_looping))
		bAllowLooping = true;

	return py_ue_new_ftransform(anim->ExtractRootMotion(start_time, delta_time, bAllowLooping));
}






#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 13)
#if !(ENGINE_MAJOR_VERSION == 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 23))
PyObject *py_ue_anim_sequence_update_compressed_track_map_from_raw(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	UAnimSequence *anim_seq = ue_py_check_type<UAnimSequence>(self);
	if (!anim_seq)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");

	anim_seq->UpdateCompressedTrackMapFromRaw();

	Py_RETURN_NONE;
}
#endif


PyObject *py_ue_anim_add_key_to_sequence(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	float frame_time;
	char *track_name;
	PyObject *py_transform;
	if (!PyArg_ParseTuple(args, "fsO:add_key_to_sequence", &frame_time, &track_name, &py_transform))
		return nullptr;

	UAnimSequence *anim_seq = ue_py_check_type<UAnimSequence>(self);
	if (!anim_seq)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");

	ue_PyFTransform *ue_py_transform = py_ue_is_ftransform(py_transform);
	if (!ue_py_transform)
		return PyErr_Format(PyExc_Exception, "argument is not a FTransform.");

	anim_seq->AddKeyToSequence(frame_time, FName(UTF8_TO_TCHAR(track_name)), ue_py_transform->transform);

	Py_RETURN_NONE;
}

PyObject *py_ue_anim_sequence_apply_raw_anim_changes(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	UAnimSequence *anim_seq = ue_py_check_type<UAnimSequence>(self);
	if (!anim_seq)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");


	if (anim_seq->DoesNeedRecompress())
	{
		anim_seq->Modify(true);
#if UE_VERSION_OLDER_THAN(5, 2, 0)
		anim_seq->RequestSyncAnimRecompression(false);
#else
		if (const ITargetPlatform* platform = GetTargetPlatformManagerRef().GetRunningTargetPlatform())
		{
			anim_seq->CacheDerivedData(platform);
		}
#endif
	}

	Py_RETURN_NONE;
}



PyObject *py_ue_anim_sequence_add_new_raw_track(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	char *name;
	PyObject *py_rast = nullptr;
	if (!PyArg_ParseTuple(args, "s|O:add_new_raw_track", &name, &py_rast))
		return nullptr;

	UAnimSequence *anim_seq = ue_py_check_type<UAnimSequence>(self);
	if (!anim_seq)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");

	FRawAnimSequenceTrack *rast = nullptr;

	if (py_rast)
	{
		ue_PyFRawAnimSequenceTrack *py_f_rast = py_ue_is_fraw_anim_sequence_track(py_rast);
		if (py_f_rast)
		{
			rast = &py_f_rast->raw_anim_sequence_track;
		}
		else
		{
			return PyErr_Format(PyExc_Exception, "argument is not a FRawAnimSequenceTrack.");
		}
	}

	anim_seq->Modify();

	FName track_name(UTF8_TO_TCHAR(name));

#if UE_VERSION_OLDER_THAN(5, 2, 0)
	const int32 index = anim_seq->GetController().AddBoneTrack(track_name);
#else
	anim_seq->GetController().AddBoneCurve(track_name);

	TArray<FName> track_names;
	anim_seq->GetDataModel()->GetBoneTrackNames(track_names);
	const int32 index = track_names.IndexOfByKey(track_name);
#endif

	anim_seq->GetController().SetBoneTrackKeys(track_name, rast->PosKeys, rast->RotKeys, rast->ScaleKeys);
	anim_seq->MarkPackageDirty();

	return PyLong_FromLong(index);
}

PyObject *py_ue_anim_sequence_update_raw_track(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	int track_index;
	PyObject *py_rast;
	if (!PyArg_ParseTuple(args, "iO:update_raw_track", &track_index, &py_rast))
		return nullptr;

	UAnimSequence *anim_seq = ue_py_check_type<UAnimSequence>(self);
	if (!anim_seq)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimSequence.");

	ue_PyFRawAnimSequenceTrack *py_f_rast = py_ue_is_fraw_anim_sequence_track(py_rast);
	if (!py_f_rast)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a FRawAnimSequenceTrack.");
	}

	anim_seq->Modify();

	TArray<FName> track_names;
	anim_seq->GetDataModel()->GetBoneTrackNames(track_names);

	const FName& track_name = track_names[track_index];
	const TArray<FVector3f>& pos_keys = py_f_rast->raw_anim_sequence_track.PosKeys;
	const TArray<FQuat4f>& rot_keys = py_f_rast->raw_anim_sequence_track.RotKeys;
	const TArray<FVector3f>& scale_keys = py_f_rast->raw_anim_sequence_track.ScaleKeys;

	anim_seq->GetController().SetBoneTrackKeys(track_name, pos_keys, rot_keys, scale_keys);

	anim_seq->MarkPackageDirty();

	Py_RETURN_NONE;
}



PyObject *py_ue_add_anim_composite_section(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	char *name;
	float time;
	if (!PyArg_ParseTuple(args, "sf:add_anim_composite_section", &name, &time))
		return nullptr;

	UAnimMontage *anim = ue_py_check_type<UAnimMontage>(self);
	if (!anim)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimMontage.");

	return PyLong_FromLong(anim->AddAnimCompositeSection(FName(UTF8_TO_TCHAR(name)), time));
}

#endif
#endif

PyObject *py_ue_anim_set_skeleton(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	PyObject *py_skeleton;
	if (!PyArg_ParseTuple(args, "O:anim_set_skeleton", &py_skeleton))
		return nullptr;

	UAnimationAsset *anim = ue_py_check_type<UAnimationAsset>(self);
	if (!anim)
		return PyErr_Format(PyExc_Exception, "UObject is not a UAnimationAsset.");

	USkeleton *skeleton = ue_py_check_type<USkeleton>(py_skeleton);
	if (!skeleton)
		return PyErr_Format(PyExc_Exception, "argument is not a USkeleton.");

	anim->SetSkeleton(skeleton);

	Py_RETURN_NONE;
}

PyObject *py_ue_get_blend_parameter(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	int index;
	if (!PyArg_ParseTuple(args, "i:get_blend_parameter", &index))
		return nullptr;

	UBlendSpace *blend = ue_py_check_type<UBlendSpace>(self);
	if (!blend)
		return PyErr_Format(PyExc_Exception, "UObject is not a UBlendSpace.");

	if (index < 0 || index > 2)
		return PyErr_Format(PyExc_Exception, "invalid Blend Parameter index");

	const FBlendParameter & parameter = blend->GetBlendParameter(index);

	return py_ue_new_owned_uscriptstruct(FBlendParameter::StaticStruct(), (uint8 *)&parameter);
}

PyObject *py_ue_set_blend_parameter(ue_PyUObject * self, PyObject * args)
{
	ue_py_check(self);

	int index;
	PyObject *py_blend;
	if (!PyArg_ParseTuple(args, "iO:get_blend_parameter", &index, &py_blend))
		return nullptr;

	UBlendSpace *blend = ue_py_check_type<UBlendSpace>(self);
	if (!blend)
		return PyErr_Format(PyExc_Exception, "UObject is not a UBlendSpace.");

	if (index < 0 || index > 2)
		return PyErr_Format(PyExc_Exception, "invalid Blend Parameter index");

	FBlendParameter *parameter = ue_py_check_struct<FBlendParameter>(py_blend);
	if (!parameter)
		return PyErr_Format(PyExc_Exception, "argument is not a FBlendParameter");

	const FBlendParameter & orig_parameter = blend->GetBlendParameter(index);

	FMemory::Memcpy((uint8 *)&orig_parameter, parameter, FBlendParameter::StaticStruct()->GetStructureSize());

	Py_RETURN_NONE;
}
