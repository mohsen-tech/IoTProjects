part of 'chips_bloc.dart';

@freezed
class ChipsState with _$ChipsState {
  const factory ChipsState.loadInProgress() = _LoadInProgress;
  const factory ChipsState.loadSuccess(List<Chip> chips) = _LoadSuccess;
  const factory ChipsState.loadFailure() = _LoadFailure;
}
