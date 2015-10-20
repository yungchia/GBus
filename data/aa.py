import urllib.request
import datetime

url = 'http://e-bus.ntpc.gov.tw/NTPCRoute/Js/RouteInfo?rid=5312&sec=0'
f = urllib.request.urlopen(url)
today_string = datetime.datetime.today().strftime('%m_%d_%H_%M_%S')
with open(today_string+".txt", "wb") as file_:
    file_.write(f.read())
    file_.close()
