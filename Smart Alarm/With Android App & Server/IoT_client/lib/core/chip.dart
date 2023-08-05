import 'package:freezed_annotation/freezed_annotation.dart';

part 'chip.freezed.dart';
part 'chip.g.dart';

@freezed
class Chip with _$Chip {
  const factory Chip({
    @JsonKey(name: 'chipID') required int id,
    required DateTime createdAt,
  }) = _Chip;

  factory Chip.fromJson(Map<String, dynamic> json) => _$ChipFromJson(json);
}
