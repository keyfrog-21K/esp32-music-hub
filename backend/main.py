from fastapi import FastAPI
from fastapi.responses import FileResponse
import subprocess
import os
from PIL import Image
import io

app = FastAPI()

# 이미지를 저장할 경로
ARTWORK_PATH = os.path.join(os.path.dirname(__file__), "artwork.jpg")
LAST_TRACK = ""

def extract_artwork():
    """AppleScript를 사용하여 Music 앱에서 앨범 아트를 추출하고 저장합니다."""
    temp_raw = os.path.join(os.path.dirname(__file__), "raw_artwork")
    script = f'''
    tell application "Music"
        if (count of artworks of current track) > 0 then
            set d to raw data of artwork 1 of current track
            set f to open for access POSIX file "{temp_raw}" with write permission
            set eof f to 0
            write d to f
            close access f
            return "success"
        else
            return "no_artwork"
        end if
    end tell
    '''
    try:
        result = subprocess.check_output(["osascript", "-e", script]).decode("utf-8").strip()
        if result == "success" and os.path.exists(temp_raw):
            # Pillow를 사용하여 이미지 리사이즈 및 변환 (240x240)
            with Image.open(temp_raw) as img:
                img = img.convert("RGB")
                img.thumbnail((240, 240))
                img.save(ARTWORK_PATH, "JPEG", quality=85)
            os.remove(temp_raw)
            return True
    except Exception as e:
        print(f"Artwork extraction error: {e}")
        if os.path.exists(temp_raw):
            os.remove(temp_raw)
    return False

def get_now_playing_info():
    global LAST_TRACK
    # AppleScript를 사용하여 Music 앱에서 정보 가져오기
    script = '''
    tell application "System Events"
        set processList to (name of every process)
    end tell

    if processList contains "Music" then
        tell application "Music"
            if player state is playing then
                set trackName to name of current track
                set artistName to artist of current track
                set albumName to album of current track
                return "playing|" & trackName & "|" & artistName & "|" & albumName
            else
                return "stopped|Not Playing||"
            end if
        end tell
    else
        return "stopped|Music app not running||"
    end if
    '''
    
    try:
        result = subprocess.check_output(["osascript", "-e", script]).decode("utf-8").strip()
        parts = result.split("|")
        if len(parts) >= 4:
            info = {
                "status": parts[0],
                "title": parts[1],
                "artist": parts[2],
                "album": parts[3]
            }
            
            # 노래가 바뀌었으면 아트워크 새로 추출
            current_track_id = f"{info['title']}-{info['artist']}"
            if info["status"] == "playing" and current_track_id != LAST_TRACK:
                if extract_artwork():
                    info["has_artwork"] = True
                    LAST_TRACK = current_track_id
                else:
                    info["has_artwork"] = False
            else:
                info["has_artwork"] = os.path.exists(ARTWORK_PATH)
                
            return info
    except Exception as e:
        print(f"Error running AppleScript: {e}")
    
    return {"status": "stopped", "title": "Error fetching info", "artist": "", "album": "", "has_artwork": False}

@app.get("/nowplaying")
async def now_playing():
    return get_now_playing_info()

@app.get("/artwork")
async def get_artwork():
    if os.path.exists(ARTWORK_PATH):
        return FileResponse(ARTWORK_PATH)
    return {"status": "error", "message": "no artwork available"}

@app.post("/control/{action}")
async def control_media(action: str):
    script_map = {
        "playpause": "tell application \"Music\" to playpause",
        "next": "tell application \"Music\" to next track",
        "prev": "tell application \"Music\" to previous track"
    }
    
    if action in script_map:
        subprocess.run(["osascript", "-e", script_map[action]])
        return {"status": "success", "action": action}
    
    return {"status": "failed", "message": "invalid action"}

if __name__ == "__main__":
    import uvicorn
    # ESP32가 접속해야 하므로 host를 0.0.0.0으로 설정
    uvicorn.run(app, host="0.0.0.0", port=8000)
