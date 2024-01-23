package com.example.proyectose

import android.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.MutableData
import com.google.firebase.database.Transaction
import com.google.firebase.database.ValueEventListener
import java.text.SimpleDateFormat
import java.util.*

class Perfil : AppCompatActivity() {

    private lateinit var databaseReference: DatabaseReference
    private lateinit var textViewSaldo: TextView
    private lateinit var textViewGreeting: TextView
    private lateinit var buttonRecargar: Button
    private lateinit var buttonRetirar: Button
    private lateinit var nombreUsuario: String
    private lateinit var recyclerViewMovimientos: RecyclerView
    private lateinit var movimientoAdapter: MovimientoAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_perfil)

        recyclerViewMovimientos = findViewById(R.id.recyclerViewMovimientos)
        recyclerViewMovimientos.layoutManager = LinearLayoutManager(this)
        movimientoAdapter = MovimientoAdapter(listOf()) // Inicializar con lista vacía o datos reales
        recyclerViewMovimientos.adapter = movimientoAdapter

        textViewGreeting = findViewById(R.id.textViewGreeting)
        textViewSaldo = findViewById(R.id.textViewSaldo)
        buttonRecargar = findViewById(R.id.buttonRecargar)
        buttonRetirar = findViewById(R.id.buttonRetirar)

        // Recibir el nombre de usuario del Intent y configurar el saludo
        nombreUsuario = intent.getStringExtra("nombreusuario").toString()

        nombreUsuario?.let {
            textViewGreeting.text = "Hola, $it"
            obtenerSaldoUsuario(it)
            obtenerMovimientosUsuario(it)
        }

        // Configurar eventos de clic para los botones
        buttonRecargar.setOnClickListener {
            mostrarDialogoRecarga()
        }

        buttonRetirar.setOnClickListener {
            mostrarDialogoRetiro()
        }

        // Configuración adicional para RecyclerView si se utiliza para mostrar movimientos
    }

    private fun obtenerSaldoUsuario(nombreUsuario: String) {
        databaseReference = FirebaseDatabase.getInstance().getReference("usuarios").child(nombreUsuario)

        databaseReference.child("saldo").addListenerForSingleValueEvent(object :
            ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                val saldo = dataSnapshot.getValue(Double::class.java)
                textViewSaldo.text = "Saldo Actual: $${saldo ?: "0.00"}"
            }

            override fun onCancelled(databaseError: DatabaseError) {
                Toast.makeText(this@Perfil, "Error al cargar datos: ${databaseError.message}", Toast.LENGTH_LONG).show()
            }
        })
    }

    private fun mostrarDialogoRecarga() {
        val dialogBuilder = AlertDialog.Builder(this)
        val inflater = this.layoutInflater
        val dialogView = inflater.inflate(R.layout.dialogo_recarga, null)
        dialogBuilder.setView(dialogView)

        val editTextMonto = dialogView.findViewById<EditText>(R.id.editTextMontoRecarga)

        dialogBuilder.setTitle("Recargar Saldo")
        dialogBuilder.setMessage("Ingresa el monto a recargar")
        dialogBuilder.setPositiveButton("Recargar") { _, _ ->
            val monto = editTextMonto.text.toString().toDoubleOrNull()
            monto?.let {
                if (it > 0) {
                    realizarRecarga(it)
                } else {
                    Toast.makeText(this@Perfil, "Ingresa un monto válido", Toast.LENGTH_SHORT).show()
                }
            }
        }
        dialogBuilder.setNegativeButton("Cancelar") { dialog, whichButton ->
            // Usuario cancela el diálogo
        }
        val b = dialogBuilder.create()
        b.show()
    }

    private fun realizarRecarga(monto: Double) {
        val dateFormat = SimpleDateFormat("yyyyMMdd-HHmmss", Locale.getDefault())
        val fechaActual = Date()
        val idMovimiento = dateFormat.format(fechaActual) + "_recarga"

        val movimiento = mapOf(
            "tipo" to "recarga",
            "monto" to monto
        )

        databaseReference.child("movimientos").child(idMovimiento).setValue(movimiento)
        databaseReference.child("saldo").runTransaction(object : Transaction.Handler {
            override fun doTransaction(mutableData: MutableData): Transaction.Result {
                val saldoActual = mutableData.getValue(Double::class.java) ?: 0.0
                mutableData.value = saldoActual + monto
                return Transaction.success(mutableData)
            }

            override fun onComplete(databaseError: DatabaseError?, success: Boolean, dataSnapshot: DataSnapshot?) {
                if (success) {
                    Toast.makeText(this@Perfil, "Recarga exitosa", Toast.LENGTH_SHORT).show()
                    obtenerSaldoUsuario(nombreUsuario)
                    obtenerMovimientosUsuario(nombreUsuario)
                } else {
                    Toast.makeText(this@Perfil, "Error al realizar la recarga", Toast.LENGTH_SHORT).show()
                }
            }
        })
    }

    private fun mostrarDialogoRetiro() {
        val dialogBuilder = AlertDialog.Builder(this)
        val inflater = this.layoutInflater
        val dialogView = inflater.inflate(R.layout.dialogo_retiro, null)
        dialogBuilder.setView(dialogView)

        val editTextMonto = dialogView.findViewById<EditText>(R.id.editTextMontoRetiro)

        dialogBuilder.setTitle("Retirar Saldo")
        dialogBuilder.setMessage("Ingresa el monto a retirar")
        dialogBuilder.setPositiveButton("Retirar") { dialog, whichButton ->
            val monto = editTextMonto.text.toString().toDoubleOrNull()
            monto?.let {
                if (it > 0) {
                    verificarYSiEsPosibleRealizarRetiro(it)
                } else {
                    Toast.makeText(this@Perfil, "Ingresa un monto válido", Toast.LENGTH_SHORT).show()
                }
            }
        }
        dialogBuilder.setNegativeButton("Cancelar") { dialog, whichButton ->
            // Usuario cancela el diálogo
        }
        val b = dialogBuilder.create()
        b.show()
    }

    private fun verificarYSiEsPosibleRealizarRetiro(monto: Double) {
        databaseReference.child("saldo").addListenerForSingleValueEvent(object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                val saldoActual = dataSnapshot.getValue(Double::class.java) ?: 0.0
                if (saldoActual >= monto) {
                    realizarRetiro(monto, saldoActual)
                } else {
                    Toast.makeText(this@Perfil, "Saldo insuficiente para retirar", Toast.LENGTH_SHORT).show()
                }
            }

            override fun onCancelled(databaseError: DatabaseError) {
                Toast.makeText(this@Perfil, "Error al cargar datos: ${databaseError.message}", Toast.LENGTH_LONG).show()
            }
        })
    }

    private fun realizarRetiro(monto: Double, saldoActual: Double) {
        val dateFormat = SimpleDateFormat("yyyyMMdd-HHmmss", Locale.getDefault())
        val fechaActual = Date()
        val idMovimiento = dateFormat.format(fechaActual) + "_retiro"

        val movimiento = mapOf(
            "tipo" to "retiro",
            "monto" to monto
        )

        databaseReference.child("movimientos").child(idMovimiento).setValue(movimiento)
        databaseReference.child("saldo").setValue(saldoActual - monto)

        // Actualizar la UI con el nuevo saldo
        obtenerSaldoUsuario(nombreUsuario)
        obtenerMovimientosUsuario(nombreUsuario)
    }

    private fun obtenerMovimientosUsuario(nombreUsuario: String) {
        databaseReference = FirebaseDatabase.getInstance().getReference("usuarios").child(nombreUsuario)

        databaseReference.child("movimientos")
            .addListenerForSingleValueEvent(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    val movimientos = mutableListOf<Movimiento>()
                    dataSnapshot.children.forEach { snapshot ->
                        val tipo = snapshot.child("tipo").getValue(String::class.java) ?: ""
                        val clave = snapshot.key ?: ""
                        val fechaHora = formatearFechaHora(clave.substringBefore('_'))
                        val monto = when (tipo) {
                            "recarga", "retiro" -> snapshot.child("monto").getValue(Double::class.java) ?: 0.0
                            "ruleta" -> {
                                val apuesta = snapshot.child("apuesta").getValue(Double::class.java) ?: 0.0
                                val ganancia = snapshot.child("ganancia").getValue(Double::class.java) ?: 0.0
                                ganancia - apuesta
                            }
                            else -> 0.0
                        }
                        Log.println(Log.INFO, monto.toString(), fechaHora)
                        movimientos.add(Movimiento(tipo, monto, fechaHora))
                    }
                    movimientos.reverse()
                    movimientoAdapter = MovimientoAdapter(movimientos)
                    recyclerViewMovimientos.adapter = movimientoAdapter
                }

                override fun onCancelled(databaseError: DatabaseError) {
                    Toast.makeText(this@Perfil, "Error al cargar movimientos: ${databaseError.message}", Toast.LENGTH_LONG).show()
                }
            })
    }

    private fun formatearFechaHora(fechaHora: String): String {
        return try {
            val formatoOriginal = SimpleDateFormat("yyyyMMdd-HHmmss", Locale.getDefault())
            val formatoNuevo = SimpleDateFormat("dd/MM/yyyy, HH:mm", Locale.getDefault())
            val fecha = formatoOriginal.parse(fechaHora)
            return formatoNuevo.format(fecha)
        } catch (e: Exception) {
            ""
        }
    }
}