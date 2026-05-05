from fastapi import FastAPI
import subprocess
import json

app = FastAPI()

def get_now_playing_info():
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
            return {
                "status": parts[0],
                "title": parts[1],
                "artist": parts[2],
                "album": parts[3]
            }
    except Exception as e:
        print(f"Error running AppleScript: {e}")
    
    return {"status": "stopped", "title": "Error fetching info", "artist": "", "album": ""}

@app.get("/nowplaying")
async def now_playing():
    return get_now_playing_info()

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
    uvicorn.run(app, host="0.0.0.0", port=8000)