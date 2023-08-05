part of 'chip_status_bloc.dart';

@freezed
class ChipStatusEvent with _$ChipStatusEvent {
  const factory ChipStatusEvent.loaded() = _Loaded;
}
