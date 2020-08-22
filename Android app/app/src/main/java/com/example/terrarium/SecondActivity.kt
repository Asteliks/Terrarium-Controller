package com.example.terrarium

import android.graphics.Color
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.util.Log
import android.widget.Toast
import com.android.volley.Request
import com.android.volley.RequestQueue
import com.android.volley.Response
import com.android.volley.toolbox.JsonArrayRequest
import com.android.volley.toolbox.Volley
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.interfaces.datasets.ILineDataSet
import kotlinx.android.synthetic.main.activity_second.*
import org.json.JSONException
import java.util.*
import java.util.Collections.swap
import kotlin.collections.ArrayList


class SecondActivity : AppCompatActivity() {
    //listy do zapisu danych
    var listTemp = ArrayList<Float>()
    var roznicaListTemp = ArrayList<Float>()
    var listWilg = ArrayList<Float>()
    var listDate_T_W = ArrayList<String>()
    var listDate_T_W2 = ArrayList<String>()
    var roznicaListWilg = ArrayList<Float>()
    var listGrz = ArrayList<Float>()
    var roznicaListGrz = ArrayList<Float>()
    var listPomp = ArrayList<Float>()
    var roznicaListPomp = ArrayList<Float>()
    var listDate_G_P = ArrayList<String>()
    var listDATY = ArrayList<String>()
    var listDATY2 = ArrayList<String>()
    var roznicaDat = ArrayList<String>()
    var roznicaDat2 = ArrayList<String>()
    //zmienne logiczne do zmiany ustawien wykresu
    var boolTemp = true
    var boolWilg = false
    var boolGrz = false
    var boolPomp = false
    var boolDay = true
    var boolMonth = false
    var boolAll = false
    private var requestQueue: RequestQueue? = null
    private var requestQueue1: RequestQueue? = null

    //pobranie daty aktualnej
    private var c = Calendar.getInstance()
    private var year = c.get(Calendar.YEAR)
    private var month = c.get(Calendar.MONTH)
    private var day = c.get(Calendar.DAY_OF_MONTH)
    // miejsca do zapisu daty
    var moja_data = ""
    var moja_data2 = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_second)
        requestQueue = Volley.newRequestQueue(this)
        requestQueue1 = Volley.newRequestQueue(this)
        jsonParseGetReadings() // odwołanie funkcji
        ButOdsDane.setOnClickListener { jsonParseGetReadings() }
        RBday.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                RBall.isChecked = false // ustawienie radiobutton na falsz
                RBmonth.isChecked = false // ustawienie radiobutton na falsz
                boolAll = false
                boolMonth = false
                boolDay = true
                setChart() // wyswietlenie wykresu
            }
        }
        RBmonth.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                RBall.isChecked = false // ustawienie radiobutton na falsz
                RBday.isChecked = false // ustawienie radiobutton na falsz
                boolAll = false
                boolDay = false
                boolMonth = true
                setChart() // wyswietlenie wykresu
            }
        }
        RBall.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                RBday.isChecked = false// ustawienie radiobutton na falsz
                RBmonth.isChecked = false// ustawienie radiobutton na falsz
                boolDay = false
                boolMonth = false
                boolAll = true
                setChart() // wyswietlenie wykresu
            }
        }
        switchTemp.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                val myToast = Toast.makeText(applicationContext, "Switch ON", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolTemp = true
                setChart() // wyswietlenie wykresu
            } else {
                val myToast = Toast.makeText(applicationContext, "Switch OFF", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolTemp = false
                setChart() // wyswietlenie wykresu
            }
        }
        switchWilg.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                val myToast = Toast.makeText(applicationContext, "Switch ON", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolWilg = true
                setChart() // wyswietlenie wykresu
            } else {
                val myToast = Toast.makeText(applicationContext, "Switch OFF", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolWilg = false
                setChart() // wyswietlenie wykresu
            }
        }
        switchPomp.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                val myToast = Toast.makeText(applicationContext, "Switch ON", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolPomp = true
                setChart() // wyswietlenie wykresu
            } else {
                val myToast = Toast.makeText(applicationContext, "Switch OFF", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolPomp = false
                setChart() // wyswietlenie wykresu
            }
        }
        switchGrz.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                val myToast = Toast.makeText(applicationContext, "Switch ON", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolGrz = true
                setChart() // wyswietlenie wykresu
            } else {
                val myToast = Toast.makeText(applicationContext, "Switch OFF", Toast.LENGTH_SHORT)
                myToast.show() // wyswietlenie krotkiej informacji  czy przycisk wlaczony/wylaczony
                boolGrz = false
                setChart() // wyswietlenie wykresu
            }
        }
    }
 //funkcja do odczytu danych z getReadings
    private fun jsonParseGetReadings() {
        //czyszczenie list
        listTemp.clear()
        listWilg.clear()
        listDate_T_W.clear()
        listDATY.clear()

        val url = "https://esp32-terrarium-control.now.sh/getReadings"
        val request =
            JsonArrayRequest(Request.Method.GET, url, null, Response.Listener { response ->
                try {

                    for (count in 0 until response.length()) {

                        val obj = response.getJSONObject(count) //uworzenie obiektu JSON z odczytanych danych
                        //uzupelnienie list odpowiednimi danymi
                        listTemp.add(obj.getString("temp").toFloat())
                        listWilg.add(obj.getString("wilg").toFloat())
                        listDate_T_W.add(obj.getString("date").substring(0, 16))
                        listDATY.add(obj.getString("date").substring(0, 10))
                    }
                    jsonParseGetStateChanges()

                    setChart()
                } catch (e: JSONException) {
                    e.printStackTrace()
                }
            }, Response.ErrorListener { error -> error.printStackTrace() })
        requestQueue?.add(request)

    }
//odczyt danych z getStateChanges
    private fun jsonParseGetStateChanges() {
        listGrz.clear()
        listPomp.clear()
        listDate_G_P.clear()
        listDATY2.clear()
        val url = "https://esp32-terrarium-control.now.sh/getStateChanges"
        val request =
            JsonArrayRequest(Request.Method.GET, url, null, Response.Listener { response ->
                try {

                    for (count in 0 until response.length()) {
                        val obj = response.getJSONObject(count) //uworzenie obiektu JSON z odczytanych danych
                        //uzupelnienie list odpowiednimi danymi
                        listGrz.add(obj.getString("grzalka").toFloat())
                        listPomp.add(obj.getString("pompka").toFloat())
                        listDate_G_P.add(obj.getString("date").substring(0, 16))
                        listDATY2.add(obj.getString("date").substring(0, 10))
                    }

                } catch (e: JSONException) {
                    e.printStackTrace()
                }
            }, Response.ErrorListener { error -> error.printStackTrace() })
        requestQueue1?.add(request)
    }
//setChart zbytnio nie ma co tam ogarniac bo to jest zrobione tak zeby dzialalo ale to jest straszne
// duzo sortowania babelkowego logika wyswietlania na wykresach i inne rzeczy ktore mozna bylo prosciej zrobic
    private fun setChart() {

        val dataSets = ArrayList<ILineDataSet>()
        if (boolAll == true) {
            roznicaDat.clear()
            listDate_T_W2.clear()
            roznicaListTemp.clear()
            roznicaListWilg.clear()
            roznicaListGrz.clear()
            roznicaListPomp.clear()
            for (count in 0 until listDATY.size) {
                roznicaDat.add(listDATY[count])
                listDate_T_W2.add(listDate_T_W[count])
                roznicaListTemp.add(listTemp[count])
                roznicaListWilg.add(listWilg[count])
            }
            for (count in 0 until listDATY2.size) {
                roznicaListGrz.add(listGrz[count])
                roznicaListPomp.add(listPomp[count])
            }
            for (i in 1 until roznicaDat.size) {
                for (j in 0 until roznicaDat.size - i) {
                    if (roznicaDat.get(j) > roznicaDat.get(j + 1)) {
                        swap(roznicaDat, j, j + 1)
                        swap(listDate_T_W2, j, j + 1)
                        swap(roznicaListTemp, j, j + 1)
                        swap(roznicaListWilg, j, j + 1)
                    }
                }
            }

        }
        if (boolDay == true) {
            listDate_T_W2.clear()
            roznicaDat.clear()
            roznicaDat2.clear()
            roznicaListTemp.clear()
            roznicaListWilg.clear()
            roznicaListGrz.clear()
            roznicaListPomp.clear()
            if (month in 0..9) {
                moja_data = year.toString() + "-0" + (month + 1).toString() + "-" + day.toString()
                moja_data2 =
                    year.toString() + "-0" + (month + 1).toString() + "-" + (day - 7).toString()
            } else {
                moja_data = year.toString() + "-" + (month + 1).toString() + "-" + day.toString()
                moja_data2 =
                    year.toString() + "-" + (month + 1).toString() + "-" + (day - 7).toString()
            }
            for (count in 0 until listDATY.size) {
                if (listDATY[count] in moja_data2..moja_data) {
                    roznicaDat.add(listDATY[count])
                    listDate_T_W2.add(listDate_T_W[count])
                    roznicaListTemp.add(listTemp[count])
                    roznicaListWilg.add(listWilg[count])

                }
            }
            for (count in 0 until listDATY2.size) {
                if (listDATY2[count] in moja_data2..moja_data) {
                    roznicaDat2.add(listDATY2[count])
                    roznicaListGrz.add(listGrz[count])
                    roznicaListPomp.add(listPomp[count])
                }
            }
            for (i in 1 until roznicaDat.size) {
                for (j in 0 until roznicaDat.size - i) {
                    if (roznicaDat.get(j) > roznicaDat.get(j + 1)) {
                        swap(roznicaDat, j, j + 1)
                        swap(listDate_T_W2, j, j + 1)
                        swap(roznicaListTemp, j, j + 1)
                        swap(roznicaListWilg, j, j + 1)

                    }
                }
            }
            for (i in 1 until roznicaDat2.size) {
                for (j in 0 until roznicaDat2.size - i) {
                    if (roznicaDat2.get(j) > roznicaDat2.get(j + 1)) {
                        swap(roznicaDat2, j, j + 1)
                        swap(roznicaListPomp, j, j + 1)
                        swap(roznicaListGrz, j, j + 1)
                    }
                }
            }

        }
        if (boolMonth == true) {
            roznicaDat.clear()
            roznicaDat2.clear()
            roznicaListTemp.clear()
            roznicaListWilg.clear()
            roznicaListGrz.clear()
            roznicaListPomp.clear()
            listDate_T_W2.clear()
            if (month in 0..9) {
                if (month == 0) {
                    moja_data =
                        year.toString() + "-0" + (month + 1).toString() + "-" + day.toString()
                    moja_data2 = year.toString() + "-" + (12).toString() + "-" + (day).toString()
                } else {
                    moja_data =
                        year.toString() + "-0" + (month + 1).toString() + "-" + day.toString()
                    moja_data2 =
                        year.toString() + "-0" + (month).toString() + "-" + (day).toString()
                }

            } else {
                moja_data = year.toString() + "-" + (month + 1).toString() + "-" + day.toString()
                moja_data2 = year.toString() + "-" + (month).toString() + "-" + (day).toString()
            }
            for (count in 0 until listDATY.size) {
                if (listDATY[count] in moja_data2..moja_data) {
                    roznicaDat.add(listDATY[count])
                    listDate_T_W2.add(listDate_T_W[count])
                    roznicaListTemp.add(listTemp[count])
                    roznicaListWilg.add(listWilg[count])

                }
            }
            for (count in 0 until listDATY2.size) {
                if (listDATY2[count] in moja_data2..moja_data) {
                    roznicaDat2.add(listDATY2[count])
                    roznicaListGrz.add(listGrz[count])
                    roznicaListPomp.add(listPomp[count])
                }
            }
            for (i in 1 until roznicaDat.size) {
                for (j in 0 until roznicaDat.size - i) {
                    if (roznicaDat.get(j) > roznicaDat.get(j + 1)) {
                        swap(roznicaDat, j, j + 1)
                        swap(listDate_T_W2, j, j + 1)
                        swap(roznicaListTemp, j, j + 1)
                        swap(roznicaListWilg, j, j + 1)

                    }
                }
            }
            for (i in 1 until roznicaDat2.size) {
                for (j in 0 until roznicaDat2.size - i) {
                    if (roznicaDat2.get(j) > roznicaDat2.get(j + 1)) {
                        swap(roznicaDat2, j, j + 1)
                        swap(roznicaListPomp, j, j + 1)
                        swap(roznicaListGrz, j, j + 1)
                    }
                }
            }
        }
        if (boolTemp == true) {
            val entriesTemp = ArrayList<Entry>()
            for (count in 0 until roznicaListTemp.size) {
                entriesTemp.add(Entry(roznicaListTemp[count], count))
            }
            val DataSetTemp = LineDataSet(entriesTemp, "Temp")
            DataSetTemp.setColor(Color.rgb(0, 255, 0))
            DataSetTemp.setDrawCircles(false)
            dataSets.add(DataSetTemp)
        }
        if (boolWilg == true) {
            val entriesWilg = ArrayList<Entry>()
            for (count in 0 until roznicaListWilg.size) {
                entriesWilg.add(Entry(roznicaListWilg[count], count))
            }
            val DataSetWilg = LineDataSet(entriesWilg, "Wilg")
            DataSetWilg.setColor(Color.rgb(0, 0, 255))
            DataSetWilg.setDrawCircles(false)
            dataSets.add(DataSetWilg)
        }
        if (boolPomp == true) {
            val entriesPomp = ArrayList<Entry>()
            for (count in 0 until roznicaListPomp.size) {
                entriesPomp.add(Entry(roznicaListPomp[count], count))
            }
            val DataSetPomp = LineDataSet(entriesPomp, "Pomp")
            DataSetPomp.setColor(Color.rgb(0, 128, 255))
            DataSetPomp.setDrawCircles(false)
            dataSets.add(DataSetPomp)
        }
        if (boolGrz == true) {
            val entriesGrz = ArrayList<Entry>()
            for (count in 0 until roznicaListGrz.size) {
                entriesGrz.add(Entry(roznicaListGrz[count], count))
            }
            val DataSetGrz = LineDataSet(entriesGrz, "Grz")
            DataSetGrz.setColor(Color.rgb(255, 0, 0))
            DataSetGrz.setDrawCircles(false)
            dataSets.add(DataSetGrz)
        }
        val labels = ArrayList<String>()
        for (count in 0 until listDate_T_W2.size) {
            labels.add("${listDate_T_W2[count]}")
        }
        val data = LineData(labels, dataSets)
        barChart.data = data
        barChart.animateY(1000)
    }

}
