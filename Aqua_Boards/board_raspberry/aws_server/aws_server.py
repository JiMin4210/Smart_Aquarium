from flask import Flask, request, render_template
from werkzeug.utils import secure_filename

app = Flask(__name__)

@app.route('/input', methods=['GET','POST'])
def input():
    if request.method == 'POST':
        f = request.files['file']

        f.save('./static/' + secure_filename(f.filename))
        return 'uploads success'


@app.route("/<pagename>")
def show(pagename):
    return render_template('show.html', image_file= pagename+".jpg")


@app.route('/')
def home():
    return 'show img test web'

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000)
~                                         