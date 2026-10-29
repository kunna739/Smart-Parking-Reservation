'use client';

import React, { useEffect, useState } from 'react';
import { api } from '@/lib/api';
import { Customer, VehicleCategory } from '@/lib/types';
import { Users, UserPlus, Car, CheckCircle2 } from 'lucide-react';

export default function CustomersPage() {
  const [customers, setCustomers] = useState<Customer[]>([]);
  const [name, setName] = useState('');
  const [phone, setPhone] = useState('');
  const [email, setEmail] = useState('');

  // Register vehicle modal/form
  const [selectedCustomerId, setSelectedCustomerId] = useState<number | null>(null);
  const [vehiclePlate, setVehiclePlate] = useState('');
  const [vehicleCategory, setVehicleCategory] = useState<VehicleCategory>('CAR');

  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [successMsg, setSuccessMsg] = useState<string | null>(null);

  const loadCustomers = async () => {
    try {
      const data = await api.getCustomers();
      setCustomers(data);
    } catch (err: any) {
      console.error(err);
    }
  };

  useEffect(() => {
    loadCustomers();
  }, []);

  const handleCreateCustomer = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!name.trim() || !phone.trim()) return;

    try {
      setLoading(true);
      setError(null);
      setSuccessMsg(null);
      await api.registerCustomer({
        name: name.trim(),
        phone: phone.trim(),
        email: email.trim(),
      });
      setSuccessMsg(`Customer ${name} registered successfully!`);
      setName('');
      setPhone('');
      setEmail('');
      loadCustomers();
    } catch (err: any) {
      setError(err.message || 'Registration failed');
    } finally {
      setLoading(false);
    }
  };

  const handleRegisterVehicle = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!selectedCustomerId || !vehiclePlate.trim()) return;

    try {
      setLoading(true);
      setError(null);
      setSuccessMsg(null);
      await api.registerVehicle({
        customer_id: selectedCustomerId,
        license_plate: vehiclePlate.trim(),
        category: vehicleCategory,
      });
      setSuccessMsg(`Vehicle ${vehiclePlate} registered and hashed in memory table!`);
      setVehiclePlate('');
      setSelectedCustomerId(null);
    } catch (err: any) {
      setError(err.message || 'Vehicle registration failed');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="space-y-8">
      <div>
        <h1 className="text-2xl font-bold text-white tracking-tight flex items-center gap-2">
          <Users className="w-6 h-6 text-indigo-400" />
          Customer &amp; Vehicle Registry
        </h1>
        <p className="text-sm text-slate-400 mt-1">
          Maintain persistent customer records and map registered vehicles to high-speed hash tables.
        </p>
      </div>

      {error && (
        <div className="p-4 bg-rose-950/40 border border-rose-800 rounded-xl text-rose-300 text-sm">
          {error}
        </div>
      )}

      {successMsg && (
        <div className="p-4 bg-emerald-950/40 border border-emerald-600/80 rounded-xl text-emerald-300 text-sm flex items-center gap-2">
          <CheckCircle2 className="w-4 h-4 shrink-0" />
          {successMsg}
        </div>
      )}

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
        {/* Create Customer Form */}
        <form
          onSubmit={handleCreateCustomer}
          className="p-6 bg-slate-900 border border-slate-800 rounded-2xl space-y-4 h-fit"
        >
          <div className="text-base font-semibold text-white border-b border-slate-800 pb-3 flex items-center gap-2">
            <UserPlus className="w-4 h-4 text-indigo-400" />
            Add New Customer
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Full Name *
            </label>
            <input
              type="text"
              required
              placeholder="e.g. Tony Stark"
              value={name}
              onChange={(e) => setName(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 focus:outline-none focus:border-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Phone Number *
            </label>
            <input
              type="text"
              required
              placeholder="e.g. +1-555-0199"
              value={phone}
              onChange={(e) => setPhone(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 font-mono focus:outline-none focus:border-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Email Address
            </label>
            <input
              type="email"
              placeholder="e.g. tony@stark.com"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 focus:outline-none focus:border-indigo-500"
            />
          </div>

          <button
            type="submit"
            disabled={loading}
            className="w-full py-2.5 bg-indigo-600 hover:bg-indigo-500 text-white rounded-xl text-sm font-semibold shadow-lg shadow-indigo-600/30 transition flex items-center justify-center gap-2"
          >
            <UserPlus className="w-4 h-4" />
            {loading ? 'Saving...' : 'Register Customer'}
          </button>
        </form>

        {/* Customer Directory Table */}
        <div className="lg:col-span-2 space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-base font-semibold text-white">
              Registered Customers ({customers.length})
            </h2>
            <span className="text-xs text-slate-400 font-mono">
              Persisted in SQLite3 customers table
            </span>
          </div>

          <div className="overflow-x-auto bg-slate-900 border border-slate-800 rounded-2xl">
            <table className="w-full text-left text-xs">
              <thead className="bg-slate-800/60 text-slate-400 uppercase tracking-wider">
                <tr>
                  <th className="py-3 px-4">ID</th>
                  <th className="py-3 px-4">Name</th>
                  <th className="py-3 px-4">Phone</th>
                  <th className="py-3 px-4">Email</th>
                  <th className="py-3 px-4 text-right">Add Vehicle</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800">
                {customers.map((c) => (
                  <tr key={c.customer_id} className="hover:bg-slate-800/40 transition">
                    <td className="py-3 px-4 font-mono font-bold text-slate-400">
                      #{c.customer_id}
                    </td>
                    <td className="py-3 px-4 font-semibold text-white">
                      {c.name}
                    </td>
                    <td className="py-3 px-4 font-mono text-slate-300">
                      {c.phone}
                    </td>
                    <td className="py-3 px-4 text-slate-400">
                      {c.email || '—'}
                    </td>
                    <td className="py-3 px-4 text-right">
                      <button
                        onClick={() => setSelectedCustomerId(c.customer_id)}
                        className="text-xs text-indigo-400 hover:text-indigo-300 underline font-medium"
                      >
                        + Add Vehicle
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>

      {/* Register Vehicle Modal */}
      {selectedCustomerId && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm p-4">
          <form
            onSubmit={handleRegisterVehicle}
            className="bg-slate-900 border border-slate-800 rounded-xl max-w-md w-full p-6 space-y-4 shadow-2xl"
          >
            <div className="flex justify-between items-center border-b border-slate-800 pb-3">
              <h3 className="text-base font-semibold text-white flex items-center gap-2">
                <Car className="w-5 h-5 text-indigo-400" />
                Register Vehicle for Customer #{selectedCustomerId}
              </h3>
              <button
                type="button"
                onClick={() => setSelectedCustomerId(null)}
                className="text-slate-400 hover:text-white"
              >
                ✕
              </button>
            </div>

            <div>
              <label className="block text-xs font-medium text-slate-400 mb-1">
                License Plate *
              </label>
              <input
                type="text"
                required
                placeholder="e.g. DL-04-EV-1001"
                value={vehiclePlate}
                onChange={(e) => setVehiclePlate(e.target.value)}
                className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 uppercase font-mono tracking-wider focus:outline-none focus:border-indigo-500"
              />
            </div>

            <div>
              <label className="block text-xs font-medium text-slate-400 mb-1">
                Vehicle Category
              </label>
              <select
                value={vehicleCategory}
                onChange={(e) => setVehicleCategory(e.target.value as VehicleCategory)}
                className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 focus:outline-none focus:border-indigo-500"
              >
                <option value="CAR">Car</option>
                <option value="BIKE">Motorcycle / Bike</option>
                <option value="EV">Electric Vehicle (EV)</option>
              </select>
            </div>

            <div className="flex justify-end gap-3 pt-2">
              <button
                type="button"
                onClick={() => setSelectedCustomerId(null)}
                className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-lg text-xs font-medium transition"
              >
                Cancel
              </button>
              <button
                type="submit"
                disabled={loading}
                className="px-4 py-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded-lg text-xs font-semibold shadow-lg shadow-indigo-600/30 transition"
              >
                {loading ? 'Registering...' : 'Save Vehicle'}
              </button>
            </div>
          </form>
        </div>
      )}
    </div>
  );
}
