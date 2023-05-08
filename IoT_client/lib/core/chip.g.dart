// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'chip.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_$_Chip _$_$_ChipFromJson(Map<String, dynamic> json) {
  return _$_Chip(
    id: json['chipID'] as int,
    createdAt: DateTime.parse(json['createdAt'] as String),
  );
}

Map<String, dynamic> _$_$_ChipToJson(_$_Chip instance) => <String, dynamic>{
      'chipID': instance.id,
      'createdAt': instance.createdAt.toIso8601String(),
    };
