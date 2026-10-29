'use client';

import React, { useEffect, useState } from 'react';
import { Slot } from '@/lib/types';
import { api } from '@/lib/api';
import {
  Car,
  Bike,
  Zap,
  X,
  Clock,
  User,
  Phone,
  CreditCard,
  Hash,
  AlertCircle,
  CheckCircle2,
  CalendarCheck,
  ArrowRight,
  ShieldCheck,
} from 'lucide-react';
import Link from 'next/link';

interface SlotDetailModalProps {
  slot: Slot | null;
  onClose: () => void;
}

export default function SlotDetailModal({ slot, onClose }: SlotDetailModalProps) {
  const [detail, setDetail] = useState<Slot | null>(slot);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (!slot) return;
    setDetail(slot);

    // Fetch fresh live details from backend
    let isMounted = true;
    const fetchLatest = async () => {
      try {
        setLoading(true);
        const fresh = await api.getSlotDetail(slot.slot_id);
        if (isMounted) setDetail(fresh);
      } catch (err) {
        // Fall back to the passed slot
      } finally {
        if (isMounted) setLoading(false);
      }
    };

    fetchLatest();
    return () => {
      isMounted = false;
    };
  }, [slot]);

  if (!slot || !detail) return null;

  const isOccupied = detail.status === 'OCCUPIED';
  const isReserved = detail.status === 'RESERVED';
  const isAvailable = detail.status === 'AVAILABLE';

  const formatDuration = (mins?: number) => {
    if (mins === undefined || mins === null || mins <= 0) return '< 1 min';
    const hours = Math.floor(mins / 60);
    const m = mins % 60;
    if (hours === 0) return `${m} minute${m === 1 ? '' : 's'}`;
    return `${hours} hr ${m} min`;
  };

  const getCategoryIcon = (cat: string) => {
    switch (cat) {
      case 'BIKE':
        return <Bike className="w-5 h-5 text-indigo-400" />;
      case 'EV':
        return <Zap className="w-5 h-5 text-emerald-400" />;
      default:
        return <Car className="w-5 h-5 text-indigo-400" />;
    }
  };

  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-in fade-in duration-200"
      onClick={onClose}
    >
      <div
        className="relative w-full max-w-lg bg-slate-900 border border-slate-800 rounded-3xl shadow-2xl overflow-hidden p-6 sm:p-7 space-y-6"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Header Bar */}
        <div className="flex items-start justify-between">
          <div className="flex items-center gap-3">
            <div className="p-3 bg-slate-800/80 border border-slate-700/60 rounded-2xl flex items-center justify-center">
              {getCategoryIcon(detail.category)}
            </div>
            <div>
              <div className="flex items-center gap-2">
                <h3 className="text-2xl font-black font-mono text-white tracking-wide">
                  {detail.slot_number}
                </h3>
                <span className="text-xs font-semibold px-2.5 py-0.5 rounded-full bg-slate-800 text-slate-300 border border-slate-700">
                  {detail.category} Space
                </span>
              </div>
              <p className="text-xs text-slate-400 mt-0.5">
                Downtown Smart Parking Facility
              </p>
            </div>
          </div>

          <button
            onClick={onClose}
            className="p-2 text-slate-400 hover:text-white bg-slate-800/60 hover:bg-slate-800 rounded-xl transition"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        {/* Status Indicator Banner */}
        <div
          className={`p-3 rounded-2xl border flex items-center justify-between text-xs font-semibold ${
            isOccupied
              ? 'bg-rose-950/40 border-rose-800/80 text-rose-300'
              : isReserved
              ? 'bg-amber-950/40 border-amber-800/80 text-amber-300'
              : 'bg-emerald-950/40 border-emerald-800/80 text-emerald-300'
          }`}
        >
          <div className="flex items-center gap-2">
            <span
              className={`w-2.5 h-2.5 rounded-full ${
                isOccupied
                  ? 'bg-rose-500 animate-pulse'
                  : isReserved
                  ? 'bg-amber-500 animate-pulse'
                  : 'bg-emerald-500'
              }`}
            />
            <span>Status: {detail.status}</span>
          </div>
          <span className="font-mono text-slate-300">
            ₹{detail.hourly_rate ?? 250}.00 / hour
          </span>
        </div>

        {/* Occupied State Details */}
        {isOccupied && (
          <div className="space-y-4">
            {/* License Plate Display */}
            <div className="p-4 bg-slate-950 border border-slate-800 rounded-2xl flex flex-col sm:flex-row items-center justify-between gap-3 text-center sm:text-left">
              <div>
                <span className="text-[10px] font-bold uppercase tracking-wider text-slate-400">
                  Parked License Plate
                </span>
                <div className="text-xl font-mono font-extrabold text-white mt-0.5">
                  {detail.license_plate || 'Unknown Plate'}
                </div>
              </div>
              <div className="px-3 py-1.5 bg-indigo-950/60 border border-indigo-700/60 rounded-xl text-indigo-300 text-xs font-semibold flex items-center gap-1.5">
                <ShieldCheck className="w-4 h-4 text-indigo-400" />
                Active Session
              </div>
            </div>

            {/* Customer & Time Information Grid */}
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 text-xs">
              {/* Customer Name */}
              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <User className="w-3.5 h-3.5 text-indigo-400" />
                  Vehicle Owner / Driver
                </span>
                <span
                  className={`font-semibold block text-sm ${
                    !detail.customer_name || detail.customer_name === 'No customer yet' || detail.customer_name === 'Walk-in Guest'
                      ? 'text-slate-400 italic'
                      : 'text-slate-100'
                  }`}
                >
                  {!detail.customer_name || detail.customer_name === 'Walk-in Guest'
                    ? 'No customer yet'
                    : detail.customer_name}
                </span>
              </div>

              {/* Contact Phone */}
              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <Phone className="w-3.5 h-3.5 text-emerald-400" />
                  Contact Details
                </span>
                <span
                  className={`font-mono block text-sm ${
                    !detail.customer_phone || detail.customer_phone === 'UNASSIGNED' || detail.customer_phone.startsWith('AUTO-')
                      ? 'text-slate-500 italic'
                      : 'text-slate-200 font-medium'
                  }`}
                >
                  {!detail.customer_phone || detail.customer_phone === 'UNASSIGNED' || detail.customer_phone.startsWith('AUTO-')
                    ? 'Not provided'
                    : detail.customer_phone}
                </span>
              </div>

              {/* Parked Duration */}
              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <Clock className="w-3.5 h-3.5 text-amber-400" />
                  Parked Duration
                </span>
                <span className="font-bold text-amber-300 block text-sm">
                  {formatDuration(detail.duration_minutes)}
                </span>
              </div>

              {/* Estimated Parking Fee */}
              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <CreditCard className="w-3.5 h-3.5 text-rose-400" />
                  Accrued Fee Due
                </span>
                <span className="font-mono font-bold text-emerald-400 block text-sm">
                  ₹{(detail.estimated_fee ?? (detail.hourly_rate ?? 250)).toFixed(2)}
                </span>
              </div>
            </div>

            {detail.booking_id && (
              <div className="p-2.5 bg-slate-950/60 rounded-xl border border-slate-800/60 text-xs text-slate-400 flex items-center justify-between font-mono">
                <span>Pre-Booking Reference</span>
                <span className="text-indigo-300 font-bold">{detail.booking_id}</span>
              </div>
            )}

            {/* Action Bar */}
            <div className="pt-2 flex items-center gap-3">
              <Link
                href="/gate"
                className="flex-1 flex items-center justify-center gap-2 px-4 py-2.5 bg-rose-600 hover:bg-rose-500 text-white rounded-xl text-xs font-semibold shadow-lg shadow-rose-600/30 transition"
              >
                Proceed to Exit Gate
                <ArrowRight className="w-4 h-4" />
              </Link>
              <button
                onClick={onClose}
                className="px-4 py-2.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl text-xs font-semibold border border-slate-700 transition"
              >
                Close
              </button>
            </div>
          </div>
        )}

        {/* Reserved State Details */}
        {isReserved && (
          <div className="space-y-4">
            <div className="p-4 bg-slate-950 border border-slate-800 rounded-2xl flex flex-col sm:flex-row items-center justify-between gap-3 text-center sm:text-left">
              <div>
                <span className="text-[10px] font-bold uppercase tracking-wider text-slate-400">
                  Confirmed Booking Reference
                </span>
                <div className="text-xl font-mono font-extrabold text-indigo-400 mt-0.5">
                  {detail.booking_id || 'RESERVED'}
                </div>
              </div>
              <div className="px-3 py-1.5 bg-amber-950/60 border border-amber-700/60 rounded-xl text-amber-300 text-xs font-semibold flex items-center gap-1.5">
                <CalendarCheck className="w-4 h-4 text-amber-400" />
                Awaiting Check-in
              </div>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 text-xs">
              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <User className="w-3.5 h-3.5 text-indigo-400" />
                  Reserved By Customer
                </span>
                <span className="font-semibold text-slate-100 block text-sm">
                  {detail.customer_name || 'Registered Customer'}
                </span>
              </div>

              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <Car className="w-3.5 h-3.5 text-emerald-400" />
                  Expected Vehicle Plate
                </span>
                <span className="font-mono font-medium text-slate-200 block text-sm">
                  {detail.license_plate || 'To be scanned at gate'}
                </span>
              </div>

              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <Phone className="w-3.5 h-3.5 text-amber-400" />
                  Contact Phone
                </span>
                <span className="font-mono text-slate-200 block text-sm">
                  {detail.customer_phone || 'Verified on Account'}
                </span>
              </div>

              <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl space-y-1">
                <span className="text-slate-400 flex items-center gap-1.5">
                  <Clock className="w-3.5 h-3.5 text-indigo-400" />
                  Reservation Hold Time
                </span>
                <span className="font-semibold text-slate-200 block text-sm">
                  Active (Guaranteed space)
                </span>
              </div>
            </div>

            {/* Action Bar */}
            <div className="pt-2 flex items-center gap-3">
              <Link
                href="/gate"
                className="flex-1 flex items-center justify-center gap-2 px-4 py-2.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded-xl text-xs font-semibold shadow-lg shadow-indigo-600/30 transition"
              >
                Check In Vehicle at Gate
                <ArrowRight className="w-4 h-4" />
              </Link>
              <button
                onClick={onClose}
                className="px-4 py-2.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl text-xs font-semibold border border-slate-700 transition"
              >
                Close
              </button>
            </div>
          </div>
        )}

        {/* Available State Details */}
        {isAvailable && (
          <div className="space-y-4">
            <div className="p-5 bg-emerald-950/30 border border-emerald-900/60 rounded-2xl text-center space-y-2">
              <div className="w-10 h-10 rounded-full bg-emerald-900/60 text-emerald-400 flex items-center justify-center mx-auto">
                <CheckCircle2 className="w-5 h-5" />
              </div>
              <h4 className="text-base font-bold text-white">
                Space is Clean & Available
              </h4>
              <p className="text-xs text-slate-400 max-w-sm mx-auto">
                Slot {detail.slot_number} is currently vacant and ready for immediate walk-in parking or advance reservation.
              </p>
            </div>

            <div className="p-3 bg-slate-800/50 border border-slate-800 rounded-xl flex items-center justify-between text-xs">
              <span className="text-slate-400">Standard Parking Rate</span>
              <span className="font-mono font-bold text-emerald-400 text-sm">
                ₹{detail.hourly_rate ?? 250}.00 / hr
              </span>
            </div>

            {/* Action Bar */}
            <div className="pt-2 flex items-center gap-3">
              <Link
                href="/reserve"
                className="flex-1 flex items-center justify-center gap-2 px-4 py-2.5 bg-emerald-600 hover:bg-emerald-500 text-white rounded-xl text-xs font-semibold shadow-lg shadow-emerald-600/30 transition"
              >
                Reserve this Space Now
                <ArrowRight className="w-4 h-4" />
              </Link>
              <button
                onClick={onClose}
                className="px-4 py-2.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl text-xs font-semibold border border-slate-700 transition"
              >
                Close
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
