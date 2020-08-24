package com.example.terrarium

import android.app.AlarmManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import android.widget.TextView
import kotlinx.android.synthetic.main.activity_main.*
import com.android.volley.Request
import com.android.volley.RequestQueue
import com.android.volley.Response
import com.android.volley.toolbox.JsonArrayRequest
import com.android.volley.toolbox.JsonObjectRequest
import com.android.volley.toolbox.Volley
import org.json.JSONException
import java.util.*
import kotlin.collections.ArrayList

class MainActivity : AppCompatActivity() {
    private lateinit var textViewTemp: TextView
    private lateinit var textViewWilg: TextView
    private lateinit var textViewTempAKT: TextView
    private lateinit var textViewWilgAKT: TextView
    private var requestQueue: RequestQueue? = null
    private var requestQueue1: RequestQueue? = null
    var listTemp = ArrayList<Float>()
    var listWilg = ArrayList<Float>()
    var listDate_T_W = ArrayList<String>()
    var jsontemp = " "
    var jsonwilg = " "
    var czy_wykonane = false
    lateinit var  alarmManager: AlarmManager
    lateinit var  alarmIntent: PendingIntent
            //////////////////////////
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        textViewTemp = findViewById(R.id.TempViewZadValue)
        textViewWilg = findViewById(R.id.WilgViewZadValue)
        textViewTempAKT = findViewById(R.id.TempViewAktValue)
        textViewWilgAKT = findViewById(R.id.WilgViewAktValue)
        requestQueue = Volley.newRequestQueue(this)
        requestQueue1 = Volley.newRequestQueue(this)
        ButSecAct.setOnClickListener{
               var nowaAktywnosc = Intent(applicationContext, SecondActivity::class.java)
               startActivity(nowaAktywnosc) // przejscie do drugiej aktywnosci po wcisnieciu przycisku
            }
        ButThiSec.setOnClickListener{
            var nowaAktywnosc = Intent(applicationContext, ThirdActivity::class.java)
            startActivity(nowaAktywnosc) // przejscie do drugiej aktywnosci po wcisnieciu przycisku
        }
        ButOdswiez.setOnClickListener {
            jsonParseGetConfig()
            jsonParseGetReadings()
            //przycisk z mozliwoscia odswiezenia danych
        }
        jsonParseGetConfig()
        jsonParseGetReadings()
    }
    override fun onStart() {
        alarmManager = getSystemService(Context.ALARM_SERVICE) as AlarmManager
        alarmIntent = PendingIntent.getBroadcast(applicationContext,0,
            Intent(applicationContext, Alarm::class.java),0)
        val handler = Handler()
        // taki licznik co uruchamia sie co minute
        handler.postDelayed(object : Runnable {
            override fun run() {
                //jesli temperaturatura znaczaco odbiega od zadanej ma byc powiadomienie co minute
                if(!jsontemp.isNullOrEmpty() && listTemp.isNotEmpty()){
                     if (!((jsontemp.toFloat()+5 >= listTemp.last()) && (jsontemp.toFloat()-5 <= listTemp.last()))){
                         alarmManager.set(AlarmManager.RTC_WAKEUP, 0, alarmIntent)
                         czy_wykonane = false
                     }
                }
                handler.postDelayed(this, 60000)//1 min
            }
        }, 0)

        val handler1 = Handler()
        // taki licznik co uruchamia sie co minute
        handler1.postDelayed(object : Runnable {
            override fun run() {
                if(czy_wykonane && !jsontemp.isNullOrEmpty() && listTemp.isNotEmpty()){
                    jsonParseGetConfig()
                    jsonParseGetReadings()
                    czy_wykonane = false
                }
                handler1.postDelayed(this, 60000)//1 min
            }
        }, 0)
        super.onStart()
    }
    override fun onResume() {
        jsonParseGetConfig()
        jsonParseGetReadings()
        super.onResume()
    }
    // funkcja  czytajaca dane z getConfig
    private fun jsonParseGetConfig(){
        val url = getString(R.string.getConfig)
        val request = JsonObjectRequest(Request.Method.GET, url, null, Response.Listener {
                response ->try {
            jsontemp = response.getString("temp")
            jsonwilg = response.getString("wilg")
            textViewTemp.setText("$jsontemp")
            textViewWilg.setText("$jsonwilg")
        } catch (e: JSONException) {
            e.printStackTrace()
        }
        }, Response.ErrorListener { error -> error.printStackTrace() })
        requestQueue?.add(request)
    }
    //funkcja pobierajaca dane z getReadings
    private fun jsonParseGetReadings(){
        listTemp.clear()
        listWilg.clear()
        listDate_T_W.clear()
        val url = getString(R.string.getReadings)
        val request = JsonArrayRequest(Request.Method.GET, url, null, Response.Listener {
                response ->try {
            for (count in 0 until response.length()) {
                val obj = response.getJSONObject(count)
                listTemp.add(obj.getString("temp").toFloat())
                listWilg.add(obj.getString("wilg").toFloat())
                listDate_T_W.add(obj.getString("date").substring(0, 16))
            }
            //sortowanie babelkowe wedlug daty
            for (i in 1 until listDate_T_W.size) {
                for (j in 0 until listDate_T_W.size - i) {
                    if (listDate_T_W.get(j) > listDate_T_W.get(j + 1)) {
                        Collections.swap(listDate_T_W, j, j + 1)
                        Collections.swap(listTemp, j, j + 1)
                        Collections.swap(listWilg, j, j + 1)

                    }
                }
            }
            //zaktualizowanie okien z ostatnia zapisana wartoscia
            textViewTempAKT.setText("${listTemp.last()}")
            textViewWilgAKT.setText("${listWilg.last()}")
            czy_wykonane = true
        } catch (e: JSONException) {
            e.printStackTrace()
        }
        }, Response.ErrorListener { error -> error.printStackTrace() })
        requestQueue1?.add(request)
    }
}
