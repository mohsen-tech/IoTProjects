part of 'chip_status_bloc.dart';

@freezed
class ChipStatusState with _$ChipStatusState {
  const factory ChipStatusState.loadInProgress() = _LoadInProgress;
  const factory ChipStatusState.loadSuccess(bool status) = _LoadSuccess;
  const factory ChipStatusState.loadFailure() = _LoadFailure;
}
