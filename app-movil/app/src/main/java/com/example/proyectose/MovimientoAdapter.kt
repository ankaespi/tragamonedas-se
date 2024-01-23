package com.example.proyectose

import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import java.util.Locale

class MovimientoAdapter(private val movimientos: List<Movimiento>) :
    RecyclerView.Adapter<MovimientoAdapter.MovimientoViewHolder>() {

    class MovimientoViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val textViewMovimiento: TextView = itemView.findViewById(R.id.textViewMovimiento)
        val textViewMonto: TextView = itemView.findViewById(R.id.textViewMonto)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MovimientoViewHolder {
        val itemView = LayoutInflater.from(parent.context).inflate(R.layout.item_movimiento, parent, false)
        return MovimientoViewHolder(itemView)
    }

    override fun onBindViewHolder(holder: MovimientoViewHolder, position: Int) {
        val movimientoActual = movimientos[position]
        "${movimientoActual.tipo.uppercase(Locale.ROOT)} - ${movimientoActual.fechaHora}".also { holder.textViewMovimiento.text = it }

        val montoTexto = when (movimientoActual.tipo) {
            "recarga" -> "+${movimientoActual.monto}"  // Agrega '+' para recargas
            "retiro" -> "-${movimientoActual.monto}"   // Agrega '-' para retiros
            "ruleta" -> {
                val resultado = movimientoActual.monto ?: 0.0
                if (resultado >= 0) "+$resultado" else "-${-resultado}"  // Agrega '+' o '-' para ruleta
            }
            else -> movimientoActual.monto.toString()
        }

        holder.textViewMonto.text = montoTexto
        holder.textViewMonto.setTextColor(
            if (montoTexto.startsWith("+")) Color.GREEN else Color.RED
        )
    }

    override fun getItemCount() = movimientos.size
}
