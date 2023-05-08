// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides

part of 'chip.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

T _$identity<T>(T value) => value;

final _privateConstructorUsedError = UnsupportedError(
    'It seems like you constructed your class using `MyClass._()`. This constructor is only meant to be used by freezed and you are not supposed to need it nor use it.\nPlease check the documentation here for more informations: https://github.com/rrousselGit/freezed#custom-getters-and-methods');

Chip _$ChipFromJson(Map<String, dynamic> json) {
  return _Chip.fromJson(json);
}

/// @nodoc
class _$ChipTearOff {
  const _$ChipTearOff();

  _Chip call(
      {@JsonKey(name: 'chipID') required int id, required DateTime createdAt}) {
    return _Chip(
      id: id,
      createdAt: createdAt,
    );
  }

  Chip fromJson(Map<String, Object> json) {
    return Chip.fromJson(json);
  }
}

/// @nodoc
const $Chip = _$ChipTearOff();

/// @nodoc
mixin _$Chip {
  @JsonKey(name: 'chipID')
  int get id => throw _privateConstructorUsedError;
  DateTime get createdAt => throw _privateConstructorUsedError;

  Map<String, dynamic> toJson() => throw _privateConstructorUsedError;
  @JsonKey(ignore: true)
  $ChipCopyWith<Chip> get copyWith => throw _privateConstructorUsedError;
}

/// @nodoc
abstract class $ChipCopyWith<$Res> {
  factory $ChipCopyWith(Chip value, $Res Function(Chip) then) =
      _$ChipCopyWithImpl<$Res>;
  $Res call({@JsonKey(name: 'chipID') int id, DateTime createdAt});
}

/// @nodoc
class _$ChipCopyWithImpl<$Res> implements $ChipCopyWith<$Res> {
  _$ChipCopyWithImpl(this._value, this._then);

  final Chip _value;
  // ignore: unused_field
  final $Res Function(Chip) _then;

  @override
  $Res call({
    Object? id = freezed,
    Object? createdAt = freezed,
  }) {
    return _then(_value.copyWith(
      id: id == freezed
          ? _value.id
          : id // ignore: cast_nullable_to_non_nullable
              as int,
      createdAt: createdAt == freezed
          ? _value.createdAt
          : createdAt // ignore: cast_nullable_to_non_nullable
              as DateTime,
    ));
  }
}

/// @nodoc
abstract class _$ChipCopyWith<$Res> implements $ChipCopyWith<$Res> {
  factory _$ChipCopyWith(_Chip value, $Res Function(_Chip) then) =
      __$ChipCopyWithImpl<$Res>;
  @override
  $Res call({@JsonKey(name: 'chipID') int id, DateTime createdAt});
}

/// @nodoc
class __$ChipCopyWithImpl<$Res> extends _$ChipCopyWithImpl<$Res>
    implements _$ChipCopyWith<$Res> {
  __$ChipCopyWithImpl(_Chip _value, $Res Function(_Chip) _then)
      : super(_value, (v) => _then(v as _Chip));

  @override
  _Chip get _value => super._value as _Chip;

  @override
  $Res call({
    Object? id = freezed,
    Object? createdAt = freezed,
  }) {
    return _then(_Chip(
      id: id == freezed
          ? _value.id
          : id // ignore: cast_nullable_to_non_nullable
              as int,
      createdAt: createdAt == freezed
          ? _value.createdAt
          : createdAt // ignore: cast_nullable_to_non_nullable
              as DateTime,
    ));
  }
}

/// @nodoc
@JsonSerializable()
class _$_Chip implements _Chip {
  const _$_Chip(
      {@JsonKey(name: 'chipID') required this.id, required this.createdAt});

  factory _$_Chip.fromJson(Map<String, dynamic> json) =>
      _$_$_ChipFromJson(json);

  @override
  @JsonKey(name: 'chipID')
  final int id;
  @override
  final DateTime createdAt;

  @override
  String toString() {
    return 'Chip(id: $id, createdAt: $createdAt)';
  }

  @override
  bool operator ==(dynamic other) {
    return identical(this, other) ||
        (other is _Chip &&
            (identical(other.id, id) ||
                const DeepCollectionEquality().equals(other.id, id)) &&
            (identical(other.createdAt, createdAt) ||
                const DeepCollectionEquality()
                    .equals(other.createdAt, createdAt)));
  }

  @override
  int get hashCode =>
      runtimeType.hashCode ^
      const DeepCollectionEquality().hash(id) ^
      const DeepCollectionEquality().hash(createdAt);

  @JsonKey(ignore: true)
  @override
  _$ChipCopyWith<_Chip> get copyWith =>
      __$ChipCopyWithImpl<_Chip>(this, _$identity);

  @override
  Map<String, dynamic> toJson() {
    return _$_$_ChipToJson(this);
  }
}

abstract class _Chip implements Chip {
  const factory _Chip(
      {@JsonKey(name: 'chipID') required int id,
      required DateTime createdAt}) = _$_Chip;

  factory _Chip.fromJson(Map<String, dynamic> json) = _$_Chip.fromJson;

  @override
  @JsonKey(name: 'chipID')
  int get id => throw _privateConstructorUsedError;
  @override
  DateTime get createdAt => throw _privateConstructorUsedError;
  @override
  @JsonKey(ignore: true)
  _$ChipCopyWith<_Chip> get copyWith => throw _privateConstructorUsedError;
}
